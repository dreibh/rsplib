/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef LEAFLINKEDREDBLACKTREE_H
#define LEAFLINKEDREDBLACKTREE_H

#include <stdio.h>
#include <stdlib.h>
#include "doublelinkedringlist.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned long long LeafLinkedRedBlackTreeNodeValueType;

enum LeafLinkedRedBlackTreeNodeColorType
{
   Red   = 1,
   Black = 2
};


struct LeafLinkedRedBlackTreeNode
{
   struct DoubleLinkedRingListNode ListNode;
   struct LeafLinkedRedBlackTreeNode*       Parent;
   struct LeafLinkedRedBlackTreeNode*       LeftSubtree;
   struct LeafLinkedRedBlackTreeNode*       RightSubtree;
   enum LeafLinkedRedBlackTreeNodeColorType Color;
   LeafLinkedRedBlackTreeNodeValueType      Value;
   LeafLinkedRedBlackTreeNodeValueType      ValueSum;  // ValueSum := LeftSubtree->Value + Value + RightSubtree->Value
};

struct LeafLinkedRedBlackTree
{
   struct LeafLinkedRedBlackTreeNode  NullNode;
   struct DoubleLinkedRingList        List;
   size_t                             Elements;
   void                               (*PrintFunction)(const void* node, FILE* fd);
   int                                (*ComparisonFunction)(const void* node1, const void* node2);
};


void leafLinkedRedBlackTreeNodeNew(struct LeafLinkedRedBlackTreeNode* node);
void leafLinkedRedBlackTreeNodeDelete(struct LeafLinkedRedBlackTreeNode* node);
int leafLinkedRedBlackTreeNodeIsLinked(struct LeafLinkedRedBlackTreeNode* node);


void leafLinkedRedBlackTreeNew(struct LeafLinkedRedBlackTree* llrbt,
                               void                    (*printFunction)(const void* node, FILE* fd),
                               int                     (*comparisonFunction)(const void* node1, const void* node2));
void leafLinkedRedBlackTreeDelete(struct LeafLinkedRedBlackTree* llrbt);
void leafLinkedRedBlackTreeVerify(struct LeafLinkedRedBlackTree* llrbt);
void leafLinkedRedBlackTreeInternalPrint(struct LeafLinkedRedBlackTree*     llrbt,
                                         struct LeafLinkedRedBlackTreeNode* node,
                                         FILE*                              fd);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalGetNearestPrev(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      struct LeafLinkedRedBlackTreeNode** root,
                                      struct LeafLinkedRedBlackTreeNode*  parent,
                                      struct LeafLinkedRedBlackTreeNode*  node);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalGetNearestNext(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      struct LeafLinkedRedBlackTreeNode** root,
                                      struct LeafLinkedRedBlackTreeNode*  parent,
                                      struct LeafLinkedRedBlackTreeNode*  node);
int leafLinkedRedBlackTreeIsEmpty(struct LeafLinkedRedBlackTree* llrbt);
void leafLinkedRedBlackTreePrint(struct LeafLinkedRedBlackTree* llrbt,
                                 FILE*                          fd);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetFirst(
                                      const struct LeafLinkedRedBlackTree* llrbt);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetLast(
                                      const struct LeafLinkedRedBlackTree* llrbt);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetPrev(
                                      const struct LeafLinkedRedBlackTree* llrbt,
                                      struct LeafLinkedRedBlackTreeNode*   node);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNext(
                                      const struct LeafLinkedRedBlackTree* llrbt,
                                      struct LeafLinkedRedBlackTreeNode*   node);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNearestPrev(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* cmpNode);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNearestNext(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* cmpNode);
size_t leafLinkedRedBlackTreeGetElements(const struct LeafLinkedRedBlackTree* llrbt);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalFindPrev(
                                      const struct LeafLinkedRedBlackTree* llrbt,
                                      struct LeafLinkedRedBlackTreeNode*   cmpNode);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalFindNext(
                                      const struct LeafLinkedRedBlackTree* llrbt,
                                      struct LeafLinkedRedBlackTreeNode*   cmpNode);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInsert(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* node);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeRemove(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* node);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeFind(
                                      const struct LeafLinkedRedBlackTree*     llrbt,
                                      const struct LeafLinkedRedBlackTreeNode* cmpNode);
LeafLinkedRedBlackTreeNodeValueType leafLinkedRedBlackTreeGetValueSum(
                                       const struct LeafLinkedRedBlackTree* llrbt);


struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNodeByValue(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      LeafLinkedRedBlackTreeNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
