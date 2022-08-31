/*******************************************************************************
 * Copyright (c) 2000, 2017 IBM Corp. and others
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

#ifndef OMR_REGION_HPP
#define OMR_REGION_HPP

#pragma once

#include <stddef.h>
#include <deque>
#include <stack>
#include "infra/ReferenceWrapper.hpp"
#include "env/TypedAllocator.hpp"
#include "env/MemorySegment.hpp"
#include "env/RawAllocator.hpp"
#include "env/PersistentAllocator.hpp"
#include <unordered_map>
#include <vector>

#define MAX_BACKTRACE_SIZE 10
#define REGION_BACKTRACE_DEPTH 3
#define TARGET_EXECUTABLE_FILE "libj9jit29.so"

template<typename K, typename V>
using PersistentUnorderedMapAllocator = TR::typed_allocator<std::pair<const K, V>, TR::PersistentAllocator &>;
template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using PersistentUnorderedMap = std::unordered_map<K, V, H, E, PersistentUnorderedMapAllocator<K, V>>;
template<typename K>
using PersistentVectorAllocator = TR::typed_allocator<K, TR::PersistentAllocator &>;
template<typename K>
using PersistentVector = std::vector<K, PersistentVectorAllocator<K>>;

struct AllocEntry
   {
   void *_trace[MAX_BACKTRACE_SIZE];

   bool operator==(const AllocEntry &other) const 
      {
         return memcmp(_trace, other._trace, sizeof(void *)*MAX_BACKTRACE_SIZE) == 0;
      }
   };

namespace std {
  template <>
  struct hash<AllocEntry>
   {
   std::size_t operator()(const AllocEntry& k) const
      {
      size_t result = 0;
      for (int i = 0; i < MAX_BACKTRACE_SIZE; i++) 
         {
         result ^= hash<void *>()(k._trace[i]);
         }
      return result;
      }
   };
}

class RegionLog
   {
   public:
      bool _isHeap;
      uint32_t _sequenceNumber;
      char *_methodCompiled;
      void *_regionTrace[REGION_BACKTRACE_DEPTH];
      // Logical timestamp for start of the region
      int32_t _startTime;
      int32_t _endTime;
      size_t _bytesAllocated;

      PersistentUnorderedMap<AllocEntry, size_t> _allocMap; // TODO: change this to actual thing instead of pointer
      RegionLog();
      ~RegionLog();
      bool operator==(const RegionLog &other) const 
         {
            return memcmp(_regionTrace, other._regionTrace, sizeof(void *)*REGION_BACKTRACE_DEPTH) == 0;
         }
      };

namespace TR {

class SegmentProvider;
class RegionProfiler;

class Region
   {

   /**
    * @brief The Destructable class provides a common base class, providing an
    * interface or invoking the destructor of allocated objects of arbitrary type.
    */
   class Destructable
      {
   public:
      Destructable(Destructable * const prev) :
         _prev(prev)
         {
         }

      Destructable * prev() { return _prev; }

      virtual ~Destructable() throw() {}

   private:
      Destructable * const _prev;
      };

   /**
    * @brief A class template allowing the creation of allocation instances of arbitrary
    * type, all of which have a predictable means of accessing their destructor.
    */
   template <typename T>
   class Instance : public Destructable
      {
   public:
      Instance(Destructable *prev, const T& value) :
         Destructable(prev),
         _instance(value)
         {
         }

      ~Instance() throw()
         {
         }

      T& value() { return _instance; }

   private:
      T _instance;
      };

public:
   Region(TR::SegmentProvider &segmentProvider, TR::RawAllocator rawAllocator, bool isHeap = true);
   Region(const Region &prototype, bool isHeap = true);
   virtual ~Region() throw();
   void * allocate(const size_t bytes, void * hint = 0);

   static void initAllocMapList(TR::PersistentAllocator *allocator);
   static void printRegionAllocations();

   // RegionLog to collect all allocations inside this Region instance
   struct RegionLog *_regionAllocMap;
   // TR::Comp() local copy
   class OMR_EXTENSIBLE Compilation *_compilation;
   // Flag for collect or not
   bool _collectStackTrace;

   /**
    * @brief A function template to create a Region-managed object instance.
    *
    * @param[in] prototype A copy-constructible instance used to initialize the Region-managed instance.
    *
    * Object instances created in this manner will be destroyed in LIFO order when the owning
    * Region is destroyed.\n
    * NOTE: If, using a region R0, a second Region R1 is instantiated (directly or indirectly)
    * via this mechanism, any objects created in R0 after the instantiation of R1 will have a
    * shorter life span than all the objects created in R1.\n
    * \n
    * Given the context of R0 and R1 above:\n
    * Objects created in R0 before R1 > Objects created in R1 > Objects created in R0 after R1\n
    * \n
    * This is most likely going to be invoked using a temporary:\n
    * ```region.create( MyClass(arg0, ...argN) );```\n
    * Or, if using the default constructor, use extra parentheses to avoid the 'most vexing parse':\n
    * ```region.create( (MyClass()) );```
    */
   template <typename T> inline T& create(const T& prototype);

   void deallocate(void * allocation, size_t = 0) throw();

   static void reset(TR::Region& targetRegion, TR::Region& prototypeRegion)
      {
      targetRegion.~Region(); 
      new (&targetRegion) TR::Region(prototypeRegion);
      }

   friend bool operator ==(const TR::Region &lhs, const TR::Region &rhs)
      {
      return &lhs == &rhs;
      }

   friend bool operator !=(const TR::Region &lhs, const TR::Region &rhs)
      {
      return !operator ==(lhs, rhs);
      }

   // Enable automatic conversion into a form compatible with C++ standard library containers
   template<typename T> operator TR::typed_allocator<T, Region& >()
      {
      return TR::typed_allocator<T, Region& >(*this);
      }

   size_t bytesAllocated() { return _bytesAllocated; }
   static size_t initialSize() { return INITIAL_SEGMENT_SIZE; }
private:
   friend class TR::RegionProfiler;

   size_t round(size_t bytes);

   size_t _bytesAllocated;
   TR::SegmentProvider &_segmentProvider;
   TR::RawAllocator _rawAllocator;
   TR::MemorySegment _initialSegment;
   TR::reference_wrapper<TR::MemorySegment> _currentSegment;

   Destructable *_lastDestructable;

   static const size_t INITIAL_SEGMENT_SIZE = 4096;

   union {
      char data[INITIAL_SEGMENT_SIZE];
      unsigned long long alignment;
      } _initialSegmentArea;
};

}

inline void * operator new(size_t size, TR::Region &region) { return region.allocate(size); }
inline void operator delete(void * p, TR::Region &region) { region.deallocate(p); }

inline void * operator new[](size_t size, TR::Region &region) { return region.allocate(size); }
inline void operator delete[](void * p, TR::Region &region) { region.deallocate(p); }

template <typename T>
T &
TR::Region::create(const T &value)
   {
   Instance<T> *instance = new (*this) Instance<T>(_lastDestructable, value);
   _lastDestructable = static_cast<Destructable *>(instance);
   return instance->value();
   }
   
#endif // OMR_REGION_HPP
