/*******************************************************************************
 * Copyright (c) 2000, 2016 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "env/MemorySegment.hpp"
#include "env/SegmentProvider.hpp"
#include "env/Region.hpp"
#include "infra/CriticalSection.hpp"
#include "infra/ReferenceWrapper.hpp"
#include "env/TRMemory.hpp"
#include "infra/Monitor.hpp"
#include "control/OMROptions.hpp"
#include "compile/Compilation.hpp"
#include <unordered_map>
#include <libunwind.h>
#include <execinfo.h>
#include <string.h>

// Note for instrumentation:
// _collectBackTrace:   0: no collection (no call to backtrace), 1: run backtrace but no insertion to universal, 
//                      2: backtrace stack only (backtrace only stack regions), 3: backtrace heap only, 
//                      4: backtrace stack and heap
// _printBackTrace:     0: nothing at the end, 1: no print but loop at the end, 2: print

// Constructor for regionLog to keep the log of each region object
regionLog::regionLog(TR::PersistentAllocator *allocator)
   {
   // in heap region, is_heap will be kept true while stack region will rewrite this to false on creation
   compInfo = NULL;
   allocMap = new (PERSISTENT_NEW) PersistentUnorderedMap<allocEntry, size_t>(PersistentUnorderedMap<allocEntry, size_t>::allocator_type(*allocator));
   }

namespace TR {

static PersistentVector<regionLog *> *heapAllocMapList = NULL;
static Monitor *heapAllocMapListMonitor;
static PersistentAllocator *_persistentAllocator = NULL;

Region::Region(TR::SegmentProvider &segmentProvider, TR::RawAllocator rawAllocator, bool isHeap) :
   _bytesAllocated(0),
   _segmentProvider(segmentProvider),
   _rawAllocator(rawAllocator),
   _initialSegment(_initialSegmentArea.data, INITIAL_SEGMENT_SIZE),
   _currentSegment(TR::ref(_initialSegment)),
   _lastDestructable(NULL)
   {
   if (OMR::Options::_collectBackTrace >= 1)
      {
      regionAllocMap = new (PERSISTENT_NEW) regionLog(_persistentAllocator);
      regionAllocMap->_isHeap = isHeap;
      // collect backtrace of the constructor of the region
      void *trace[REGION_BACKTRACE_DEPTH + 1];
      unw_backtrace(trace, REGION_BACKTRACE_DEPTH + 1);
      memcpy(regionAllocMap->regionTrace, &trace[1], REGION_BACKTRACE_DEPTH * sizeof(void *));
      }
   }

Region::Region(const Region &prototype, bool isHeap) :
   _bytesAllocated(0),
   _segmentProvider(prototype._segmentProvider),
   _rawAllocator(prototype._rawAllocator),
   _initialSegment(_initialSegmentArea.data, INITIAL_SEGMENT_SIZE),
   _currentSegment(TR::ref(_initialSegment)),
   _lastDestructable(NULL)
   {
   if (OMR::Options::_collectBackTrace >= 1)
      {
      regionAllocMap = new (PERSISTENT_NEW) regionLog(_persistentAllocator);
      regionAllocMap->_isHeap = isHeap;
      // collect backtrace of the constructor of the region
      void *trace[REGION_BACKTRACE_DEPTH + 1];
      unw_backtrace(trace, REGION_BACKTRACE_DEPTH + 1);
      memcpy(regionAllocMap->regionTrace, &trace[1], REGION_BACKTRACE_DEPTH * sizeof(void *));
      }
   }

Region::~Region() throw()
   {
      if (OMR::Options::_collectBackTrace >= 2)
         {
         // add heapAllocMap to heapAllocMapList
         TR_ASSERT(heapAllocMapList && heapAllocMapListMonitor, "heapAllocMapList unintialized");
         OMR::CriticalSection listInsertCS(heapAllocMapListMonitor);
         heapAllocMapList->push_back(regionAllocMap);
         regionAllocMap = NULL;
         }
   /*
    * Destroy all object instances that depend on the region
    * to manage their lifetimes.
    */
   Destructable *lastDestructable = _lastDestructable;
   while (lastDestructable)
      {
      Destructable * const currentDestructable = lastDestructable;
      lastDestructable = currentDestructable->prev();
      currentDestructable->~Destructable();
      }

   for (
      TR::reference_wrapper<TR::MemorySegment> latestSegment(_currentSegment);
      latestSegment.get() != _initialSegment;
      latestSegment = _currentSegment
      )
      {
      _currentSegment = TR::ref(latestSegment.get().unlink());
      _segmentProvider.release(latestSegment);
      }
   TR_ASSERT(_currentSegment.get() == _initialSegment, "self-referencial link was broken");
   }

void *
Region::allocate(size_t const size, void *hint)
   {
   if (OMR::Options::_collectBackTrace >= 1)
      {
      // Add compilation information to regionAllocMap
      if (regionAllocMap->compInfo == NULL)
         {
         if (TR::comp())
            {
            size_t length = strlen(TR::comp()->signature()) + 1;
            regionAllocMap->compInfo = (char *) _persistentAllocator->allocate(length);
            memcpy(regionAllocMap->compInfo, TR::comp()->signature(), length);
            }
         }
      // Backtrace allocation
      if (OMR::Options::_collectBackTrace >= 4 || (OMR::Options::_collectBackTrace == 2 && !regionAllocMap->_isHeap) || (OMR::Options::_collectBackTrace == 3 && regionAllocMap->_isHeap))
         {
         struct allocEntry entry;
         void *trace[MAX_BACKTRACE_SIZE + 1];
         unw_backtrace(trace, MAX_BACKTRACE_SIZE + 1);
         memcpy(entry.trace, &trace[1], MAX_BACKTRACE_SIZE * sizeof(void *));
         TR_ASSERT(regionAllocMap, "regionAllocMap is not built");
         auto match = regionAllocMap->allocMap->find(entry);
         if (match != regionAllocMap->allocMap->end())
            {
            match->second += size;
            }
         else
            {
            regionAllocMap->allocMap->insert({entry, size});
            }
         }
      }
   size_t const roundedSize = round(size);
   if (_currentSegment.get().remaining() >= roundedSize)
      {
      _bytesAllocated += roundedSize;
      return _currentSegment.get().allocate(roundedSize);
      }
   TR::MemorySegment &newSegment = _segmentProvider.request(roundedSize);
   TR_ASSERT(newSegment.remaining() >= roundedSize, "Allocated segment is too small");
   newSegment.link(_currentSegment.get());
   _currentSegment = TR::ref(newSegment);
   _bytesAllocated += roundedSize;
   return _currentSegment.get().allocate(roundedSize);
   }

void
Region::deallocate(void * allocation, size_t) throw()
   {
   }

size_t
Region::round(size_t bytes)
   {
   return (bytes+15) & (~15);
   }

void
Region::initAllocMapList(TR::PersistentAllocator *allocator)
   {
      if (OMR::Options::_collectBackTrace >= 1)
      {
      _persistentAllocator = allocator;
      heapAllocMapList = new (PERSISTENT_NEW) PersistentVector<regionLog *>(PersistentVector<regionLog *>::allocator_type(*allocator));
      heapAllocMapListMonitor = Monitor::create("JITCompilerHeapAllocMapListMonitor");
      }
   }

static void
put_offset(std::FILE *file, char *line)
   {
   if (strstr(line, TARGET_EXECUTABLE_FILE) != NULL)
      {
      // Warning: here we make the assumption that the line is of the format <executable_file>(+0x<offset>)
      // without checking the format, doing below lines is risky
      // We make such assumption to reduce the cost of checking each time for a faster output.
      *strchr(line, ')') = '\0';
      fprintf(file, "%s ", strchr(line, '(') + 4);
      }
   }

void
Region::printRegionAllocations() 
   {
   if (OMR::Options::_printBackTrace > 0)
      {
      TR_ASSERT_FATAL(heapAllocMapList, "heapAllocMapList is not initialized");
      std::FILE *out_file = fopen((const char *)OMR::Options::_backTraceFileName,"w");
      for (auto &region : *heapAllocMapList) 
         {
         // Here we assume backTraceFile is initialized.
         if (OMR::Options::_printBackTrace == 2)  
            {
            // fflush(stdout);
            // Output to selected file in the format:
            // Compiled method's info
            // Region backtrace (3 lines)
            // Allocated size for heap
            if ((!region->_isHeap && OMR::Options::_collectBackTrace == 3) || (region->_isHeap && OMR::Options::_collectBackTrace == 2))
               {
               continue;
               }
            if (region->compInfo)
               {
               fprintf(out_file, "%s\n", region->compInfo);
               }
            else
               {
               // no signature means no allocation, skip the empty entry
               continue;
               }
            // 0 for stack, 1 for heap
            if (region->_isHeap)
               {
               fprintf(out_file, "1 ");
               }
            else
               {
               fprintf(out_file, "0 ");
               }
            char **temp = backtrace_symbols((void **)region->regionTrace, REGION_BACKTRACE_DEPTH);
            for (int i = 0; i < REGION_BACKTRACE_DEPTH; i++)
               {
               put_offset(out_file, temp[i]);
               }
            fprintf(out_file, "\n");
            for (auto &allocPair : *(region->allocMap))
               {
               fprintf(out_file, "%zu ", allocPair.second);
               temp = backtrace_symbols((void **)allocPair.first.trace, MAX_BACKTRACE_SIZE);
               for (int i = 0; i < MAX_BACKTRACE_SIZE; i++)
                  {
                  put_offset(out_file, temp[i]);
                  }
               fprintf(out_file, "\n");
               }
            }
         }
      fclose(out_file);
      }
   }
}