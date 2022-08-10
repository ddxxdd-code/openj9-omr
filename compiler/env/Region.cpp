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
#include <unordered_map>
#include <libunwind.h>
#include <execinfo.h>

// Constructor for regionLog to keep the log of each region object
regionLog::regionLog(TR::PersistentAllocator *allocator)
   {
   // in heap region, is_heap will be kept true while stack region will rewrite this to false on creation
   is_heap = true;
   // collect backtrace of the constructor of the region
   regionTraceSize = unw_backtrace(regionTrace, MAX_BACKTRACE_SIZE);
   compInfo = NULL;
   allocMap = new (PERSISTENT_NEW) PersistentUnorderedMap<allocEntry, size_t>(PersistentUnorderedMap<allocEntry, size_t>::allocator_type(*allocator));
   }

namespace TR {

// static PersistentUnorderedMap<size_t, void *> *heapAllocMapList = NULL;
static PersistentUnorderedMap<size_t, regionLog *> *heapAllocMapList = NULL;
static Monitor *heapAllocMapListMonitor;
static PersistentAllocator *_persistentAllocator = NULL;
size_t total_method_compiled = 0;

Region::Region(TR::SegmentProvider &segmentProvider, TR::RawAllocator rawAllocator) :
   _bytesAllocated(0),
   _segmentProvider(segmentProvider),
   _rawAllocator(rawAllocator),
   _initialSegment(_initialSegmentArea.data, INITIAL_SEGMENT_SIZE),
   _currentSegment(TR::ref(_initialSegment)),
   _lastDestructable(NULL)
   {
   if (OMR::Options::_collectBackTrace >= 1)
      {
      // OMR::CriticalSection mapAllocCS(heapAllocMapListMonitor);
      heapAllocMap = new (PERSISTENT_NEW) regionLog(_persistentAllocator);
      // heapAllocMap->allocMap = new (PERSISTENT_NEW) PersistentUnorderedMap<allocEntry, size_t>(PersistentUnorderedMap<allocEntry, size_t>::allocator_type(*_persistentAllocator));
      // heapAllocMapMonitor = Monitor::create("JITCompilerHeapAllocMapMonitor");
      }
   }

Region::Region(const Region &prototype) :
   _bytesAllocated(0),
   _segmentProvider(prototype._segmentProvider),
   _rawAllocator(prototype._rawAllocator),
   _initialSegment(_initialSegmentArea.data, INITIAL_SEGMENT_SIZE),
   _currentSegment(TR::ref(_initialSegment)),
   _lastDestructable(NULL)
   {
   if (OMR::Options::_collectBackTrace >= 1)
      {
      heapAllocMap = new (PERSISTENT_NEW) regionLog(_persistentAllocator);
      // heapAllocMap = new (PERSISTENT_NEW) struct regionLog;
      // heapAllocMap->allocMap = new (PERSISTENT_NEW) PersistentUnorderedMap<allocEntry, size_t>(PersistentUnorderedMap<allocEntry, size_t>::allocator_type(*_persistentAllocator));
      // heapAllocMap = new (PERSISTENT_NEW) PersistentUnorderedMap<regionLog, size_t>(PersistentUnorderedMap<regionLog, size_t>::allocator_type(*_persistentAllocator));
      // OMR::CriticalSection mapAllocCS(heapAllocMapListMonitor);
      // heapAllocMap = new (PERSISTENT_NEW) PersistentUnorderedMap<allocEntry, size_t>(PersistentUnorderedMap<allocEntry, size_t>::allocator_type(*_persistentAllocator));
      // heapAllocMapMonitor = Monitor::create("JITCompilerHeapAllocMapMonitor");
      }
   }

Region::~Region() throw()
   {
      if (OMR::Options::_collectBackTrace >= 1)
      {
         // add heapAllocMap to heapAllocMapList
         if (heapAllocMapList && heapAllocMapListMonitor)
            {
            OMR::CriticalSection listInsertCS(heapAllocMapListMonitor);
            heapAllocMapList->insert({total_method_compiled, heapAllocMap});
            }
         else
            {
            printf("heapAllocMapList unintialized\n");
            }    
      total_method_compiled += 1;
      heapAllocMap = NULL;
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
      // Add compilation information to regionLog
      
      // Backtrace heap allocation
      // printf("Heap allocate [%u] bytes\n", size);
      // fflush(stdout);
      // struct allocEntry *entry = new(PERSISTENT_NEW) allocEntry;
      struct allocEntry entry;
      entry.traceSize = unw_backtrace(entry.trace, MAX_BACKTRACE_SIZE);
      // entry->traceSize = backtrace(entry->trace, MAX_BACKTRACE_SIZE);
      // OMR::CriticalSection cs(heapAllocMapMonitor);
      if (heapAllocMap)
         {
         auto match = heapAllocMap->allocMap->find(entry);
         if (match != heapAllocMap->allocMap->end())
            {
            match->second += size;
            }
         else
            {
            heapAllocMap->allocMap->insert({entry, size});
            }
         }
      else
         {
         printf("heapAllocMap is not built\n");
         }
      // backtrace_symbols_fd(entry->trace, entry->traceSize, fileno(stdout));
      // printf("=== end ===\n");
      // fflush(stdout);
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
Region::init_alloc_map_list(TR::PersistentAllocator *allocator)
   {
      if (OMR::Options::_collectBackTrace >= 1)
      {
      _persistentAllocator = allocator;
      heapAllocMapList = new (PERSISTENT_NEW) PersistentUnorderedMap<size_t, regionLog *>(PersistentUnorderedMap<size_t, regionLog *>::allocator_type(*allocator));
      // heapAllocMapList = new (PERSISTENT_NEW) PersistentUnorderedMap<size_t, PersistentUnorderedMap<allocEntry, size_t> *>(PersistentUnorderedMap<size_t, PersistentUnorderedMap<allocEntry, size_t> *>::allocator_type(*allocator));
      // heapAllocMapList = new (PERSISTENT_NEW) PersistentUnorderedMap<size_t, void *>(PersistentUnorderedMap<size_t, void *>::allocator_type(*allocator));
      heapAllocMapListMonitor = Monitor::create("JITCompilerHeapAllocMapListMonitor");
      }
   }

void
Region::print_alloc_entry() 
   {
   if (OMR::Options::_noPrintBackTrace > 0)
      {
      if (!heapAllocMapList) {
         printf("no map to print\n");
         return;
      }
      for (auto &pair : *heapAllocMapList) 
         {
         if (OMR::Options::_noPrintBackTrace == 2)
            {
            // fflush(stdout);
            printf("Method [%lu]\n", pair.first);
            // fflush(stdout);
            for (auto &heapAllocPair : *(pair.second->allocMap))
               {
               if (pair.second->is_heap)
                  {
                  printf("Heap Allocated [%lu] bytes\n", heapAllocPair.second);
                  }
               else
                  {
                  printf("Stack Allocated [%lu] bytes\n", heapAllocPair.second);
                  }
               // fflush(stdout);
               backtrace_symbols_fd((void **)heapAllocPair.first.trace, heapAllocPair.first.traceSize, fileno(stdout));
               // fflush(stdout);
               }
            printf("=== End ===\n");
            fflush(stdout);
            }
         else
            {
               for (auto &heapAllocPair : *(pair.second->allocMap))
               {
               // printf("Heap Allocated [%lu] bytes\n", heapAllocPair.second);
               // fflush(stdout);
               char **temp = backtrace_symbols((void **)heapAllocPair.first.trace, heapAllocPair.first.traceSize);
               // fflush(stdout);
               }
            }
         }
      }
   }
}