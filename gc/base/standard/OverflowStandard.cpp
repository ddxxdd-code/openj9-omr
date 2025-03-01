/*******************************************************************************
 * Copyright (c) 1991, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "omr.h"

#include "OverflowStandard.hpp"

#include "CollectorLanguageInterface.hpp"
#include "MarkMap.hpp"
#include "MarkingScheme.hpp"
#include "ParallelGlobalGC.hpp"
#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"
#include "Heap.hpp"
#include "Packet.hpp"
#include "ParallelTask.hpp"
#include "WorkPackets.hpp"

#include "HeapRegionIterator.hpp"
#include "ObjectHeapIteratorAddressOrderedList.hpp"
/**
 * Create a new MM_OverflowStandard object
 */
MM_OverflowStandard *
MM_OverflowStandard::newInstance(MM_EnvironmentBase *env, MM_WorkPackets *workPackets)
{
	MM_OverflowStandard *overflow = (MM_OverflowStandard *)env->getForge()->allocate(sizeof(MM_OverflowStandard), OMR::GC::AllocationCategory::FIXED, OMR_GET_CALLSITE());
	if (NULL != overflow) {
		new(overflow) MM_OverflowStandard(env,workPackets);
		if (!overflow->initialize(env)) {
			overflow->kill(env);
			overflow = NULL;
		}
	}
	return overflow;
}

bool
MM_OverflowStandard::initialize(MM_EnvironmentBase *env)
{
	return MM_WorkPacketOverflow::initialize(env);
}

void
MM_OverflowStandard::tearDown(MM_EnvironmentBase *env)
{
	MM_WorkPacketOverflow::tearDown(env);
}

void
MM_OverflowStandard::emptyToOverflow(MM_EnvironmentBase *env, MM_Packet *packet, MM_OverflowType type)
{
	MM_ParallelGlobalGC *collector = (MM_ParallelGlobalGC *)_extensions->getGlobalCollector();
	void *objectPtr = NULL;

	_overflow = true;

	/* Broadcast the overflow to the concurrent collector so it can take any remedial action */
	collector->workStackOverflow();

	_extensions->globalGCStats.workPacketStats.setSTWWorkStackOverflowOccured(true);
	_extensions->globalGCStats.workPacketStats.incrementSTWWorkStackOverflowCount();
	_extensions->globalGCStats.workPacketStats.setSTWWorkpacketCountAtOverflow(_workPackets->getActivePacketCount());

	/* Empty the current packet by setting its overflow bit (double marking) */
	while(NULL != (objectPtr = packet->pop(env))) {
		overflowItemInternal(env, objectPtr);
	}

	Assert_MM_true(packet->isEmpty());
}

void
MM_OverflowStandard::overflowItem(MM_EnvironmentBase *env, void *item, MM_OverflowType type)
{
	MM_ParallelGlobalGC *collector = (MM_ParallelGlobalGC *)_extensions->getGlobalCollector();
	_overflow = true;

	/* Broadcast the overflow to the concurrent collector so it can take any remedial action */
	collector->workStackOverflow();

	_extensions->globalGCStats.workPacketStats.setSTWWorkStackOverflowOccured(true);
	_extensions->globalGCStats.workPacketStats.incrementSTWWorkStackOverflowCount();
	_extensions->globalGCStats.workPacketStats.setSTWWorkpacketCountAtOverflow(_workPackets->getActivePacketCount());

	overflowItemInternal(env, item);
}

void
MM_OverflowStandard::overflowItemInternal(MM_EnvironmentBase *env, void *item)
{
	void *heapBase = _extensions->heap->getHeapBase();
	void *heapTop = _extensions->heap->getHeapTop();

	if ((PACKET_ARRAY_SPLIT_TAG != ((uintptr_t)item & PACKET_ARRAY_SPLIT_TAG)) &&  (item >= heapBase) && (item < heapTop)) {
		MM_ParallelGlobalGC *globalCollector = (MM_ParallelGlobalGC *)_extensions->getGlobalCollector();
		MM_MarkingScheme *markingScheme = globalCollector->getMarkingScheme();
		MM_MarkMap *markMap = markingScheme->getMarkMap();
		omrobjectptr_t objectPtr = (omrobjectptr_t)item;

		/* object has to be marked already */
		Assert_MM_true(markMap->isBitSet(objectPtr));
		Assert_MM_false(markMap->isBitSet((omrobjectptr_t)((uintptr_t)item + markMap->getObjectGrain())));

		/* set overflow bit (double marking) */
		markMap->atomicSetBit((omrobjectptr_t)((uintptr_t)item + markMap->getObjectGrain()));

		/* Perform language specific actions */
		markingScheme->getMarkingDelegate()->handleWorkPacketOverflowItem(env,objectPtr);
	}
}

void
MM_OverflowStandard::fillFromOverflow(MM_EnvironmentBase *env, MM_Packet *packet)
{
	Assert_MM_unreachable();
}

void
MM_OverflowStandard::handleOverflow(MM_EnvironmentBase *env)
{

	if (env->_currentTask->synchronizeGCThreadsAndReleaseMain(env, UNIQUE_ID)) {
		_overflow = false;

		MM_Heap *heap = _extensions->heap;
		MM_HeapRegionManager *regionManager = heap->getHeapRegionManager();
		GC_HeapRegionIterator regionIterator(regionManager);
		MM_HeapRegionDescriptor *region = NULL;
		MM_ParallelGlobalGC *globalCollector = (MM_ParallelGlobalGC *)_extensions->getGlobalCollector();
		MM_MarkingScheme *markingScheme = globalCollector->getMarkingScheme();
		MM_MarkMap *markMap = markingScheme->getMarkMap();

		while((region = regionIterator.nextRegion()) != NULL) {
			GC_ObjectHeapIteratorAddressOrderedList objectIterator(_extensions, region, false);
			omrobjectptr_t object;

			while((object = objectIterator.nextObject()) != NULL) {
				/* search for double marked (overflowed) objects */
				if ((markMap->isBitSet(object)) && (markMap->isBitSet((omrobjectptr_t)((uintptr_t)object + markMap->getObjectGrain())))) {
					/* clean overflow mark */
					markMap->clearBit((omrobjectptr_t)((uintptr_t)object + markMap->getObjectGrain()));

					/* scan overflowed object */
					markingScheme->scanObject(env, object, SCAN_REASON_OVERFLOWED_OBJECT);
				}
			}
		}
		env->_currentTask->releaseSynchronizedGCThreads(env);
	}
}

void
MM_OverflowStandard::reset(MM_EnvironmentBase *env)
{

}

bool
MM_OverflowStandard::isEmpty()
{
	return true;
}
