/*******************************************************************************
 * Copyright (c) 2000, 2022 IBM Corp. and others
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

#ifndef SINKSTORES_INCL
#define SINKSTORES_INCL

#include <stddef.h>
#include <stdint.h>
#include "compile/Compilation.hpp"
#include "env/TRMemory.hpp"
#include "il/Node.hpp"
#include "infra/Flags.hpp"
#include "infra/List.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"

class TR_BitVector;
class TR_HashTab;
class TR_LiveOnAllPaths;
class TR_LiveVariableInformation;
class TR_Liveness;
class TR_MovableStore;
class TR_SinkStores;
namespace TR { class Block; }
namespace TR { class CFGEdge; }
namespace TR { class CFGNode; }
namespace TR { class RegisterMappedSymbol; }
namespace TR { class SymbolReference; }
namespace TR { class TreeTop; }

// Store Sinking

class TR_LiveOnNotAllPaths
   {
   public:

   static void *operator new(size_t size, TR::Allocator a)
      { return a.allocate(size); }
   static void  operator delete(void *ptr, TR::Allocator a)
      {
      // If there is an exception thrown during construction, the compilation
      // will be aborted, and all memory associated with that compilation will get freed.
      }
   static void  operator delete(void *ptr, size_t size)
      { ((TR_LiveOnNotAllPaths*)ptr)->allocator().deallocate(ptr, size); } /* t->allocator() must return the same allocator as used for new */

   /* Virtual destructor is necessary for the above delete operator to work
    * See "Modern C++ Design" section 4.7
    */
   virtual ~TR_LiveOnNotAllPaths() {}

   TR_LiveOnNotAllPaths(TR::Compilation *comp, TR_Liveness *liveOnSomePaths, TR_LiveOnAllPaths *liveOnAllPaths);

   TR::Compilation *   comp()          { return _comp; }

   TR_Memory *          trMemory()      { return comp()->trMemory(); }
   TR_StackMemory       trStackMemory() { return trMemory(); }

   TR::Allocator        allocator()     { return comp()->allocator(); }

   TR::Compilation * _comp;

   int32_t        _numNodes;
   int32_t        _numLocals;
   TR_BitVector **_inSetInfo;
   TR_BitVector **_outSetInfo;
   };

class TR_UseOrKillInfo
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)

   TR_UseOrKillInfo(TR::TreeTop *tt, TR::Block *block, TR_BitVector *usedSymbols, TR_BitVector *commonedSymbols, uint32_t &indirectLoadCount, TR_BitVector *killedSymbols) :
                   _tt(tt),
                   _block(block),
                   _usedSymbols(usedSymbols),
                   _commonedSymbols(commonedSymbols),
                   _indirectLoadCount(indirectLoadCount),
                   _killedSymbols(killedSymbols),
                   _movableStore(NULL){}

   TR::TreeTop      *_tt;
   uint32_t       _indirectLoadCount;
   TR::Block        *_block;
   TR_BitVector    *_usedSymbols;
   TR_BitVector    *_commonedSymbols;
   TR_BitVector    *_killedSymbols;
   TR_MovableStore *_movableStore;  // all TR_MovableStore's are also TR_UseOrKillInfo's so this is a pointer
                                    // to the corresponding movableStore (if one exists)
   };

class TR_MovableStore
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)

   TR_MovableStore(TR_SinkStores *s,
                   TR_UseOrKillInfo *useOrKillInfo,
                   int32_t symIdx = -1,
                   TR_BitVector *commonedLoadsUnderTree = 0,
                   TR_BitVector *commonedLoadsAfter = 0,
                   int32_t depth = 0,
                   TR_BitVector *needTempForCommonedLoads = 0,
                    TR_BitVector *satisfiedCommonedLoads = 0);

   TR_UseOrKillInfo *_useOrKillInfo;
   int32_t       _symIdx;
   TR_BitVector *_commonedLoadsUnderTree;   // commoned used syms under this store in the IL tree (syms are not first used)
                                            // - syms that have associated temps are excluded (only when enablePreciseSymbolTracking() == false otherwise all syms are included)
   TR_BitVector *_commonedLoadsAfter;       // used syms which are commoned following the store (syms are first used and are commoned later)
   TR::Compilation * comp() { return _comp; }

   TR_Memory *    trMemory()      { return comp()->trMemory(); }
   TR_StackMemory trStackMemory() { return trMemory(); }

   TR::Compilation * _comp;

   TR_SinkStores * _s;
   int32_t       _depth;                    // a measure of store tree complexity
   bool          _movable;
   TR_BitVector *_needTempForCommonedLoads; // move stores with commoned load
   bool          _isLoadStatic;             // is this a store of a static load?

   };

class TR_StoreInformation
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)
   TR_StoreInformation(TR::TreeTop *store, bool copy, bool needsDuplication = true) :
                        _store(store), _copy(copy), _needsDuplication(needsDuplication), _storeTemp(NULL) { }

   TR::TreeTop *_store;   // original store to be sunk
   TR::TreeTop *_storeTemp;// dup store with commoned loads replaced by temp
   bool        _copy;    // whether original store should be copied or moved
                         //  it should be copied if the store is placed in a block that does NOT extend the original source block
                         //  it should be moved if it is to be placed in a block that extends the original source block
   bool        _needsDuplication;  // has the store already been duplicated by sinkStorePlacement or should it be done in doSinking
   };

class TR_EdgeInformation
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)

   TR_EdgeInformation(TR::CFGEdge *edge, TR_BitVector *symbols) { _edge = edge; _symbolsUsedOrKilled = symbols; }

   TR::CFGEdge   *_edge;                  // edge where store will be placed
   TR_BitVector *_symbolsUsedOrKilled;   // symbols that are used or killed by stores placed along this edge
   };

class TR_EdgeStorePlacement
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)

   TR_EdgeStorePlacement(TR_StoreInformation *store, TR_EdgeInformation *edge, TR_Memory * m)
      : _stores(m), _edges(m)
      { _stores.add(store); _edges.add(edge); }

   List<TR_StoreInformation> _stores; // stores to place here
   List<TR_EdgeInformation>  _edges;  // list of cfg edges to _block that should be split to place def
                                      // NOTE: only one of these edges need be split: others can simply be redirected to the new split block
   };

class TR_BlockStorePlacement
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)

   TR_BlockStorePlacement(TR_StoreInformation *store, TR::Block *block, TR_Memory * m)
      : _stores(m)
      { _stores.add(store); _block = block; }

   List<TR_StoreInformation> _stores; // stores to place here
   TR::Block                 *_block;  // block where stores will be placed (at beginning)
   };


typedef List<TR_BlockStorePlacement> TR_BlockStorePlacementList;
typedef List<TR_EdgeStorePlacement> TR_EdgeStorePlacementList;

class TR_BlockListEntry
   {
   public:
   TR_ALLOC(TR_Memory::DataFlowAnalysis)

   TR_BlockListEntry(TR::Block *block, TR::CFGEdge *pred, TR::Region &region)
      : _preds(region)
      { _block = block; if (pred != 0) _preds.add(pred); _count = 1; }

   TR::Block       * _block;   // block
   List<TR::CFGEdge> _preds;   // list of edges from which we reached block
   int32_t          _count;   // cached count of _preds entries
   };

class TR_OrderedBlockList : public List<TR_BlockListEntry>
   {
   public:
   TR_OrderedBlockList(TR_Memory * m) : List<TR_BlockListEntry>(m) { }
   TR_ALLOC(TR_Memory::DataFlowAnalysis)
   ListElement<TR_BlockListEntry> *addInTraversalOrder(TR::Block *block, bool forward, TR::CFGEdge *edge);
   bool removeBlockFromList(TR::Block *block, TR::CFGEdge *edge);
   };

class TR_SinkStores : public TR::Optimization
   {
   public:
   // abstract base class for store sinking implementations
   // sinkStorePlacement and storeIsSinkingCandidate must be implemented
   //
   TR_SinkStores(TR::OptimizationManager *manager);

   bool usesDataFlowAnalysis()          {return _storeSinkingFlags.testAny(UsesDataFlowAnalysis);}
   void setUsesDataFlowAnalysis(bool b) {_storeSinkingFlags.set(UsesDataFlowAnalysis, b);}

   bool isExceptionFlagIsSticky()         {return _storeSinkingFlags.testAny(ExceptionFlagIsSticky);}
   void setExceptionFlagIsSticky(bool b)  {_storeSinkingFlags.set(ExceptionFlagIsSticky, b);}

   bool sinkStoresWithStaticLoads()          {return _storeSinkingFlags.testAny(SinkStoresWithStaticLoads);}
   void setSinkStoresWithStaticLoads(bool b) {_storeSinkingFlags.set(SinkStoresWithStaticLoads, b);}

   virtual TR::RegisterMappedSymbol *getSinkableSymbol(TR::Node *node);

   private:
   virtual void lookForSinkableStores();
   virtual void doSinking();
   TR_EdgeInformation *findEdgeInformation(TR::CFGEdge *edge, List<TR_EdgeInformation> & edgeList);

   protected:
   virtual bool storeIsSinkingCandidate(TR::Block *block,
                                        TR::Node *node,
                                        int32_t symIdx,
                                        bool sinkIndirectLoads,
                                        uint32_t &indirectLoadCount,
                                        int32_t &depth,
                                        bool &isLoadStatic,
                                        vcount_t &treeVisitCount,
                                        vcount_t &highVisitCount) = 0;
   virtual bool sinkStorePlacement(TR_MovableStore *store, bool nextStoreWasMoved) = 0;
   void coalesceSimilarEdgePlacements();
   void placeStoresAlongEdges(List<TR_StoreInformation> & stores, List<TR_EdgeInformation> & edges);
   void placeStoresInBlock(List<TR_StoreInformation> & stores, TR::Block *placementBlock);
   virtual int32_t performStoreSinking();
   virtual bool storeCanMoveThroughBlock(TR_BitVector *blockKilledSet, TR_BitVector *blockUsedSet, int32_t symIdx, TR_BitVector *allBlockUsedSymbols = NULL, TR_BitVector *allBlockKilledSymbols = NULL);
   virtual bool treeIsSinkableStore(TR::Node *node, bool sinkIndirectLoads, uint32_t &indirectLoadCount, int32_t &depth, bool &isLoadStatic, vcount_t visitCount);
   bool checkLiveMergingPaths(TR_BlockListEntry *blockEntry, int32_t symIdx);
   bool shouldSinkStoreAlongEdge(int symIdx, TR::CFGNode *block, TR::CFGNode *succBlock, int32_t sourceBlockFrequency, bool isLoadStatic, vcount_t visitCount, TR_BitVector *allEdgeInfoUsedOrKilledSymbols);
   void recordPlacementForDefAlongEdge(TR_EdgeStorePlacement *edgePlacement);
   void recordPlacementForDefInBlock(TR_BlockStorePlacement *blockPlacement);
   bool isSafeToSinkThruEdgePlacement(int symIdx, TR::CFGNode *block, TR::CFGNode *succBlock, TR_BitVector *allEdgeInfoUsedOrKilledSymbols);
   bool isSymUsedInEdgePlacement(TR::CFGNode *block, TR::CFGNode *succBlock);

   int32_t genHandlerIndex() { return _handlerIndex++; }

   // The following hold the dataflow information needed to sink stores
   TR_LiveVariableInformation    * _liveVarInfo;
   TR_LiveOnAllPaths             * _liveOnAllPaths;
   TR_Liveness                   * _liveOnSomePaths;
   TR_LiveOnNotAllPaths          * _liveOnNotAllPaths;

   // Set of blocks that have some symbols that are live on not all paths at their exits
   TR_BitVector                  * _candidateBlocks;

   // One bit vector per block in the method describing which symbols are used or killed
   TR_BitVector                 ** _symbolsUsedInBlock;
   TR_BitVector                 ** _symbolsKilledInBlock;
//   TR_BitVector                 ** _symbolsExceptionUsedInBlock;
//   TR_BitVector                 ** _symbolsExceptionKilledInBlock;

   // These bit vectors hold the symbols defined and used by the candidate for sinking
   // Right now, there's only one tree (a store) represented in these, but in future the
   //   information for a set of trees could be put into these vectors (imagine moving an
   //   entire loop or an if-then-else statement).
   TR_BitVector                  * _usedSymbolsToMove;
   TR_BitVector                  * _killedSymbolsToMove;

   // These lists hold pending placements for sunk stores
   // They are used to detect safe movements after stores are sunk (because sunk stores need to stop other stores)
   TR_EdgeStorePlacementList    ** _placementsForEdgesToBlock;
   TR_BlockStorePlacementList   ** _placementsForBlock;

   // These hold all the store placements for blocks and edges
   TR_EdgeStorePlacementList       _allEdgePlacements;
   TR_BlockStorePlacementList      _allBlockPlacements;

   int32_t                          _handlerIndex;

   // stats tracking
   uint64_t                         _numRemovedStores;
   uint64_t                         _numPlacements;
   uint64_t                         _numTemps;
   uint64_t                         _searchMarkCalls;
   uint64_t                         _searchMarkWalks;
   uint64_t                         _killMarkWalks;
   uint64_t                         _numTransformations;

   // Data and routines needed to generate and place temp stores of killed locals
   TR_HashTab                      * _tempSymMap;
   TR::SymbolReference              * findTempSym(TR::Node *load);
   void                            replaceLoadsWithTempSym(TR::Node *newNode, TR::Node *origNode, TR_BitVector *needTempForCommonedLoads);
   bool                            isCorrectCommonedLoad(TR::Node *commonedLoad, TR::Node *searchNode);
   void                            genStoreToTempSyms(TR::TreeTop *storeLocation,
                                                      TR::Node *node,
                                                      TR_BitVector *allCommonedLoads,
                                                      TR_BitVector *killedLiveCommonedLoads,
                                                      TR::Node *store,
                                                      List<TR_MovableStore> &potentiallyMovableStores);

   // Tuning parameters controlled by env vars
   bool                            _sinkAllStores;        // all sinkable stores will be moved regardless of condition
   bool                            _printSinkStoreStats;  // print number of stores removed / sunk / temp created
   bool                            _sinkThruException;    // sink stores thru the exception edges
   int32_t                         _firstSinkOptTransformationIndex;
   int32_t                         _lastSinkOptTransformationIndex;

   enum
      {
      UsesDataFlowAnalysis                     = 0x0001,
      // AVAILABLE                             = 0x0002,
      // AVAILABLE                             = 0x0004,
      ExceptionFlagIsSticky                    = 0x0008,
      SinkStoresWithStaticLoads                = 0x0010,

      LastDummyEnum
      };
   bool                            performThisTransformation();
   flags16_t                       _storeSinkingFlags;
   };

class TR_GeneralSinkStores : public TR_SinkStores
   {
   public:
   // performs store sinking using data flow analysis
   // Uses the information computed by LiveOnAllPaths and LiveOnNotAllPaths
   // to sink stores onto paths where they are always used.  Stores (defs)
   // that are already live on all paths are not moved.
   //
   TR_GeneralSinkStores(TR::OptimizationManager *manager);
   static TR::Optimization *create(TR::OptimizationManager *manager)
      {
      return new (manager->allocator()) TR_GeneralSinkStores(manager);
      }

   virtual int32_t perform();
   virtual const char * optDetailString() const throw();

   private:
   virtual bool storeIsSinkingCandidate(TR::Block *block,
                                        TR::Node *node,
                                        int32_t symIdx,
                                        bool sinkIndirectLoads,
                                        uint32_t &indirectLoadCount,
                                        int32_t &depth,
                                        bool &isLoadStatic,
                                        vcount_t &treeVisitCount,
                                        vcount_t &highVisitCount);
   virtual bool sinkStorePlacement(TR_MovableStore *store, bool nextStoreWasMoved);
   };

// current call store with commoned load with be moved with temp because current
// store sinking code cannot push store with commoned load properly (problem with using _usedSymbolsToMove
// outside Ebb)
// TODO: make this an instance variable and not dep on ZEMUL flag to clean it up
#define MIN_TREE_DEPTH_TO_MOVE     3                      // if temp is used to sink the store, this is the min depth that can be benefit from sinking
// TODO: make this an instance variable and not dep on ZEMUL flag to clean it up
#define MAX_TREE_DEPTH_TO_DUP     12                      // set a max depth when is needed duplication so massive uncommoning does not occur in the duplicated tree
#define MAX_TREE_DEPTH_TO_MOVE    12                      // this is the max depth for movable store

#endif
