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

#ifndef LEAFLINKEDBINARYTREE_H
#define LEAFLINKEDBINARYTREE_H

#include <stdlib.h>
#include "debug.h"
#include "doublelinkedringlist.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned long long LeafLinkedBinaryTreeNodeValueType;


struct LeafLinkedBinaryTreeNode
{
   struct DoubleLinkedRingListNode   ListNode;
   struct LeafLinkedBinaryTreeNode*  Parent;
   struct LeafLinkedBinaryTreeNode*  LeftSubtree;
   struct LeafLinkedBinaryTreeNode*  RightSubtree;
   LeafLinkedBinaryTreeNodeValueType Value;
   LeafLinkedBinaryTreeNodeValueType ValueSum;  /* ValueSum := LeftSubtree->Value + Value + RightSubtree->Value */
};

struct LeafLinkedBinaryTree
{
   struct LeafLinkedBinaryTreeNode* TreeRoot;
   struct LeafLinkedBinaryTreeNode  NullNode;
   struct DoubleLinkedRingList      List;
   size_t                           Elements;
   void                             (*PrintFunction)(const void* node, FILE* fd);
   int                              (*ComparisonFunction)(const void* node1, const void* node2);
};


void leafLinkedBinaryTreeNodeNew(struct LeafLinkedBinaryTreeNode* node);
void leafLinkedBinaryTreeNodeDelete(struct LeafLinkedBinaryTreeNode* node);
int leafLinkedBinaryTreeNodeIsLinked(struct LeafLinkedBinaryTreeNode* node);


void leafLinkedBinaryTreeNew(struct LeafLinkedBinaryTree* llbt,
                             void                    (*printFunction)(const void* node, FILE* fd),
                             int                     (*comparisonFunction)(const void* node1, const void* node2));
void leafLinkedBinaryTreeDelete(struct LeafLinkedBinaryTree* llbt);
void leafLinkedBinaryTreeVerify(struct LeafLinkedBinaryTree* llbt);
void leafLinkedBinaryTreeInternalPrint(struct LeafLinkedBinaryTree*     llbt,
                                       struct LeafLinkedBinaryTreeNode* node,
                                       FILE*                            fd);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalGetNearestPrev(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  parent,
                                    struct LeafLinkedBinaryTreeNode*  node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalGetNearestNext(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  parent,
                                    struct LeafLinkedBinaryTreeNode*  node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalInsert(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  parent,
                                    struct LeafLinkedBinaryTreeNode*  node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalRemove(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  node);


int leafLinkedBinaryTreeIsEmpty(struct LeafLinkedBinaryTree* llbt);
void leafLinkedBinaryTreePrint(struct LeafLinkedBinaryTree* llbt,
                               FILE*                        fd);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetFirst(const struct LeafLinkedBinaryTree* llbt);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetLast(const struct LeafLinkedBinaryTree* llbt);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetPrev(const struct LeafLinkedBinaryTree* llbt,
                                                             struct LeafLinkedBinaryTreeNode*   node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNext(const struct LeafLinkedBinaryTree* llbt,
                                                             struct LeafLinkedBinaryTreeNode*   node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNearestPrev(
                                    struct LeafLinkedBinaryTree*     llbt,
                                    struct LeafLinkedBinaryTreeNode* cmpNode);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNearestNext(
                                    struct LeafLinkedBinaryTree*     llbt,
                                    struct LeafLinkedBinaryTreeNode* cmpNode);
size_t leafLinkedBinaryTreeGetElements(const struct LeafLinkedBinaryTree* llbt);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalFindPrev(const struct LeafLinkedBinaryTree* llbt,
                                                                      struct LeafLinkedBinaryTreeNode*   cmpNode);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalFindNext(const struct LeafLinkedBinaryTree* llbt,
                                                                      struct LeafLinkedBinaryTreeNode*   cmpNode);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInsert(struct LeafLinkedBinaryTree*     llbt,
                                                            struct LeafLinkedBinaryTreeNode* node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeRemove(struct LeafLinkedBinaryTree*     llbt,
                                                            struct LeafLinkedBinaryTreeNode* node);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeFind(const struct LeafLinkedBinaryTree*     llbt,
                                                          const struct LeafLinkedBinaryTreeNode* cmpNode);
LeafLinkedBinaryTreeNodeValueType leafLinkedBinaryTreeGetValueSum(
                                     const struct LeafLinkedBinaryTree* llbt);
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNodeByValue(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    LeafLinkedBinaryTreeNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
