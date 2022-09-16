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
// _minOptLevelCollected Collect only compilations where opt_level >= _minOptLevelCollected, default=0

// Constructor for regionLog to keep the log of each region object
RegionLog::RegionLog() :
   _allocMap(PersistentUnorderedMap<AllocEntry, size_t>::allocator_type(TR::Compiler->persistentAllocator())),
   _methodCompiled(NULL),
   _bytesSegmentProviderAllocated(0),
   _bytesSegmentProviderFreed(0),
   _bytesSegmentProviderInUseAllocated(0),
   _bytesSegmentProviderInUseFreed(0),
   _bytesSegmentProviderRealInUseAllocated(0),
   _bytesSegmentProviderRealInUseFreed(0)
   {
   }

RegionLog::~RegionLog()
   {
   if (_methodCompiled)
      {
      TR::Compiler->persistentAllocator().deallocate(_methodCompiled);
      }
   }

namespace TR {

static PersistentVector<RegionLog *> *heapAllocMapList = NULL;
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
   _collectStackTrace = false;
   if (OMR::Options::_collectBackTrace >= 1)
      {
      if (_compilation = TR::comp())
         {
         _compilation->recordRegion();
         if (_compilation->getOptLevel() >= OMR::Options::_minOptLevelCollected)
            {
            _collectStackTrace = true;
            _regionAllocMap = new (PERSISTENT_NEW) RegionLog;
            _regionAllocMap->_isHeap = isHeap;
            // collect backtrace of the constructor of the region
            void *trace[REGION_BACKTRACE_DEPTH + 1];
            unw_backtrace(trace, REGION_BACKTRACE_DEPTH + 1);
            memcpy(_regionAllocMap->_regionTrace, &trace[1], REGION_BACKTRACE_DEPTH * sizeof(void *));
            _regionAllocMap->_sequenceNumber = _compilation->getSequenceNumber();
            size_t length = strlen(_compilation->signature()) + 1;
            _regionAllocMap->_methodCompiled = (char *) TR::Compiler->persistentAllocator().allocate(length);
            memcpy(_regionAllocMap->_methodCompiled, _compilation->signature(), length);
            _regionAllocMap->_methodCompiled[length-1] = '\0';
            _regionAllocMap->_startTime = _compilation->recordEvent();
            _compilation->recordRegion();
            }
         }
      else
         {
         // Case of main heap region as main heap region is constructed using segmentProvider and rawAllocator.
         _collectStackTrace = true;
         _regionAllocMap = new (PERSISTENT_NEW) RegionLog;
         _regionAllocMap->_isHeap = isHeap;
         void *trace[REGION_BACKTRACE_DEPTH + 1];
         unw_backtrace(trace, REGION_BACKTRACE_DEPTH + 1);
         memcpy(_regionAllocMap->_regionTrace, &trace[1], REGION_BACKTRACE_DEPTH * sizeof(void *));
         _regionAllocMap->_startTime = 0;
         }
      }
   }

Region::Region(const Region &prototype, OMR::Compilation *comp, bool isHeap) :
   _bytesAllocated(0),
   _segmentProvider(prototype._segmentProvider),
   _rawAllocator(prototype._rawAllocator),
   _initialSegment(_initialSegmentArea.data, INITIAL_SEGMENT_SIZE),
   _currentSegment(TR::ref(_initialSegment)),
   _lastDestructable(NULL)
   {
   _collectStackTrace = false;
   if (OMR::Options::_collectBackTrace >= 1)
      {
      if (_compilation = TR::comp())
         {
         if (_compilation->getOptLevel() >= OMR::Options::_minOptLevelCollected)
            {
            _collectStackTrace = true;
            _regionAllocMap = new (PERSISTENT_NEW) RegionLog;
            _regionAllocMap->_isHeap = isHeap;
            // collect backtrace of the constructor of the region
            void *trace[REGION_BACKTRACE_DEPTH + 1];
            unw_backtrace(trace, REGION_BACKTRACE_DEPTH + 1);
            memcpy(_regionAllocMap->_regionTrace, &trace[1], REGION_BACKTRACE_DEPTH * sizeof(void *));
            _regionAllocMap->_sequenceNumber = _compilation->getSequenceNumber();
            size_t length = strlen(_compilation->signature()) + 1;
            _regionAllocMap->_methodCompiled = (char *) TR::Compiler->persistentAllocator().allocate(length);
            memcpy(_regionAllocMap->_methodCompiled, _compilation->signature(), length);
            _regionAllocMap->_methodCompiled[length-1] = '\0';
            _regionAllocMap->_startTime = _compilation->recordEvent();
            _compilation->recordRegion();
            }
         }
      else
         {
         // Case of alias region
         if (comp && comp->getOptLevel() < OMR::Options::_minOptLevelCollected)
            {
            _collectStackTrace = false;
            }
         else
            {
            _collectStackTrace = true;
            _regionAllocMap = new (PERSISTENT_NEW) RegionLog;
            _regionAllocMap->_isHeap = isHeap;
            void *trace[REGION_BACKTRACE_DEPTH + 1];
            unw_backtrace(trace, REGION_BACKTRACE_DEPTH + 1);
            memcpy(_regionAllocMap->_regionTrace, &trace[1], REGION_BACKTRACE_DEPTH * sizeof(void *));
            _regionAllocMap->_startTime = comp->recordEvent();
            comp->recordRegion();
            }
         }
      }
   }

Region::~Region() throw()
   {
   /*
    * Destroy all object instances that depend on the region
    * to manage their lifetimes.
    */
   size_t preReleaseBytesAllocated = _segmentProvider.bytesAllocated();
   size_t preReleaseBytesInUse = _segmentProvider.regionBytesInUse();
   size_t preReleaseBytesRealInUse = _segmentProvider.regionRealBytesInUse();
   Destructable *lastDestructable = _lastDestructable;
   while (lastDestructable)
      {
      Destructable * const currentDestructable = lastDestructable;
      lastDestructable = currentDestructable->prev();
      currentDestructable->~Destructable();
      }
   if (_collectStackTrace && OMR::Options::_collectBackTrace >= 2)
      {
      if (_compilation)
         {
         _regionAllocMap->_endTime = _compilation->recordEvent();
         _compilation->removeRegion();
         }
      else
         {
         _regionAllocMap->_endTime = -1;
         printf("destruction without compilation associated\n");
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
      size_t postReleaseBytesAllocated = _segmentProvider.bytesAllocated();
      size_t postReleaseBytesInUse = _segmentProvider.regionBytesInUse();
      size_t postReleaseBytesRealInUse = _segmentProvider.regionRealBytesInUse();
      _regionAllocMap->_bytesSegmentProviderFreed += preReleaseBytesAllocated - postReleaseBytesAllocated;
      _regionAllocMap->_bytesSegmentProviderInUseFreed += preReleaseBytesInUse - postReleaseBytesInUse;
      _regionAllocMap->_bytesSegmentProviderRealInUseFreed += preReleaseBytesRealInUse - postReleaseBytesRealInUse;

      // Get total bytes allocated
      _regionAllocMap->_bytesAllocated = bytesAllocated();
      // add heapAllocMap to heapAllocMapList
      TR_ASSERT(heapAllocMapList && heapAllocMapListMonitor, "heapAllocMapList unintialized");
      OMR::CriticalSection listInsertCS(heapAllocMapListMonitor);
      heapAllocMapList->push_back(_regionAllocMap);
      _regionAllocMap = NULL;
      }
   else
      {
      for (
      TR::reference_wrapper<TR::MemorySegment> latestSegment(_currentSegment);
      latestSegment.get() != _initialSegment;
      latestSegment = _currentSegment
      )
         {
         _currentSegment = TR::ref(latestSegment.get().unlink());
         _segmentProvider.release(latestSegment);
         }
      }
   TR_ASSERT(_currentSegment.get() == _initialSegment, "self-referencial link was broken");
   }

void *
Region::allocate(size_t const size, void *hint)
   {
   size_t const roundedSize = round(size);
   if (_collectStackTrace && roundedSize > 0)
      {
      // Add compilation information to regionAllocMap for main heap region
      if (_collectStackTrace && _regionAllocMap->_methodCompiled == NULL)
         {
         if (_compilation = TR::comp())
            {
            if (_compilation->getOptLevel() >= OMR::Options::_minOptLevelCollected)
               {
               _regionAllocMap->_sequenceNumber = _compilation->getSequenceNumber();
               size_t length = strlen(_compilation->signature()) + 1;
               _regionAllocMap->_methodCompiled = (char *) TR::Compiler->persistentAllocator().allocate(length);
               memcpy(_regionAllocMap->_methodCompiled, _compilation->signature(), length);
               _regionAllocMap->_methodCompiled[length-1] = '\0';
               printf("before main heap region, segment size: %zu\n", _segmentProvider.bytesAllocated());
               _compilation->recordRegion();
               }
            else
               {
               // Not for collected, discard this region's regionLog
               _collectStackTrace = false;
               _regionAllocMap->~RegionLog();
               TR::Compiler->persistentAllocator().deallocate(_regionAllocMap);
               }
            }
         }
         // Backtrace allocation, only collect specified regions: 2: stack only, 3: heap only, 4: all scratch memory
      if (OMR::Options::_collectBackTrace >= 4 || (OMR::Options::_collectBackTrace == 2 && !_regionAllocMap->_isHeap) || (OMR::Options::_collectBackTrace == 3 && _regionAllocMap->_isHeap))
         {
         struct AllocEntry entry;
         void *trace[MAX_BACKTRACE_SIZE + 1];
         unw_backtrace(trace, MAX_BACKTRACE_SIZE + 1);
         memcpy(entry._trace, &trace[1], MAX_BACKTRACE_SIZE * sizeof(void *));
         TR_ASSERT(_regionAllocMap, "regionAllocMap is not built");
         auto match = _regionAllocMap->_allocMap.find(entry);
         if (match != _regionAllocMap->_allocMap.end())
            {
            match->second += roundedSize;
            }
         else
            {
            _regionAllocMap->_allocMap.insert({entry, roundedSize});
            }
         }
      }
   // TODO: get current allocated from _segmentProvider, get difference after, accumulate to region
   size_t preRequestBytesAllocated = _segmentProvider.bytesAllocated();
   size_t preRequestBytesInUse = _segmentProvider.regionBytesInUse();
   size_t preRequestBytesRealInUse = _segmentProvider.regionRealBytesInUse();
   if (_currentSegment.get().remaining() >= roundedSize)
      {
      _bytesAllocated += roundedSize;
      return _currentSegment.get().allocate(roundedSize);
      }
   TR::MemorySegment &newSegment = _segmentProvider.request(roundedSize, true);
   if (_collectStackTrace && roundedSize > 0)
      {
      size_t postRequestBytesAllocated = _segmentProvider.bytesAllocated();
      size_t postRequestBytesInUse = _segmentProvider.regionBytesInUse();
      size_t postRequestBytesRealInUse = _segmentProvider.regionRealBytesInUse();
      _regionAllocMap->_bytesSegmentProviderAllocated += postRequestBytesAllocated - preRequestBytesAllocated;
      _regionAllocMap->_bytesSegmentProviderInUseAllocated += postRequestBytesInUse - preRequestBytesInUse;
      _regionAllocMap->_bytesSegmentProviderRealInUseAllocated += postRequestBytesRealInUse - preRequestBytesRealInUse;
      }
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
      heapAllocMapList = new (PERSISTENT_NEW) PersistentVector<RegionLog *>(PersistentVector<RegionLog *>::allocator_type(*allocator));
      heapAllocMapListMonitor = Monitor::create("JITCompilerHeapAllocMapListMonitor");
      }
   }

static void
putOffset(std::FILE *file, char *line)
   {
   if (char *targetExecFileStart = strstr(line, TARGET_EXECUTABLE_FILE))
      {
      // Warning: here we make the assumption that the line is of the format <executable_file>(+0x<offset>)
      // without checking the format, doing below lines is risky
      // We make such assumption to reduce the cost of checking each time for a faster output.
      char *offsetStart = strchr(targetExecFileStart, '(') + 4;   // Add 4 here is to skip prefix '(+0x'
      *strchr(offsetStart, ')') = '\0';
      fprintf(file, "%s ", offsetStart);
      }
   }

void
Region::printRegionAllocations() 
   {
   if (OMR::Options::_printBackTrace > 0)
      {
      TR_ASSERT_FATAL(heapAllocMapList, "heapAllocMapList is not initialized");
      std::FILE *out_file = fopen(OMR::Options::_backTraceFileName,"w");
      if (!out_file)
         {
         perror("open output file");
         }
      TR_ASSERT_FATAL(out_file, "output file not opened successfully");
      for (auto &region : *heapAllocMapList) 
         {
         // Here we assume backTraceFile is initialized.
         if (OMR::Options::_printBackTrace == 2)  
            {
            // Output to selected file in the format:
            // Compiled method's info
            // Region backtrace (3 lines)
            // Allocated size for heap
            if ((!region->_isHeap && OMR::Options::_collectBackTrace == 3) || (region->_isHeap && OMR::Options::_collectBackTrace == 2))
               {
               continue;
               }
            if (region->_methodCompiled)
               {
               fprintf(out_file, "%s %d %d %d %zu %zu %zu %zu %zu %zu %zu\n", 
               region->_methodCompiled, region->_sequenceNumber, region->_startTime, region->_endTime, region->_bytesAllocated, 
               region->_bytesSegmentProviderAllocated, region->_bytesSegmentProviderFreed, 
               region->_bytesSegmentProviderInUseAllocated, region->_bytesSegmentProviderInUseFreed,
               region->_bytesSegmentProviderRealInUseAllocated, region->_bytesSegmentProviderRealInUseFreed); // TODO: change printf to signify that _sequenceNumber is uint32_t and optLevel is int32_t
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
            char **temp = backtrace_symbols((void **)region->_regionTrace, REGION_BACKTRACE_DEPTH);
            for (int i = 0; i < REGION_BACKTRACE_DEPTH; i++)
               {
               putOffset(out_file, temp[i]);
               }
            free(temp);
            fprintf(out_file, "\n");
            for (auto &allocPair : region->_allocMap)
               {
               fprintf(out_file, "%zu ", allocPair.second);
               temp = backtrace_symbols((void **)allocPair.first._trace, MAX_BACKTRACE_SIZE);
               for (int i = 0; i < MAX_BACKTRACE_SIZE; i++)
                  {
                  putOffset(out_file, temp[i]);
                  }
               free(temp);
               fprintf(out_file, "\n");
               }
            }
         }
      fclose(out_file);
      }
   }
}