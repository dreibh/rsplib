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

#ifndef BINARYTREE_H
#define BINARYTREE_H

#include <stdlib.h>
#include "debug.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned long long BinaryTreeNodeValueType;


struct BinaryTreeNode
{
   struct BinaryTreeNode*  Parent;
   struct BinaryTreeNode*  LeftSubtree;
   struct BinaryTreeNode*  RightSubtree;
   BinaryTreeNodeValueType Value;
   BinaryTreeNodeValueType ValueSum;  // ValueSum := LeftSubtree->Value + Value + RightSubtree->Value
};

struct BinaryTree
{
   struct BinaryTreeNode* TreeRoot;
   struct BinaryTreeNode  NullNode;
   size_t                 Elements;
   void                   (*PrintFunction)(const void* node, FILE* fd);
   int                    (*ComparisonFunction)(const void* node1, const void* node2);
};


void binaryTreeNodeNew(struct BinaryTreeNode* node);
void binaryTreeNodeDelete(struct BinaryTreeNode* node);
int binaryTreeNodeIsLinked(struct BinaryTreeNode* node);

void binaryTreeNew(struct BinaryTree* bt,
                   void               (*printFunction)(const void* node, FILE* fd),
                   int                (*comparisonFunction)(const void* node1, const void* node2));
void binaryTreeDelete(struct BinaryTree* bt);
void binaryTreeVerify(struct BinaryTree* bt);
void binaryTreeInternalPrint(struct BinaryTree*     bt,
                             struct BinaryTreeNode* node,
                             FILE*                  fd);
struct BinaryTreeNode* binaryTreeInternalGetNearestPrev(struct BinaryTree*      bt,
                                                        struct BinaryTreeNode** root,
                                                        struct BinaryTreeNode*  parent,
                                                        struct BinaryTreeNode*  node);
struct BinaryTreeNode* binaryTreeInternalGetNearestNext(struct BinaryTree*      bt,
                                                        struct BinaryTreeNode** root,
                                                        struct BinaryTreeNode*  parent,
                                                        struct BinaryTreeNode*  node);
struct BinaryTreeNode* binaryTreeInternalInsert(struct BinaryTree*      bt,
                                                struct BinaryTreeNode** root,
                                                struct BinaryTreeNode*  parent,
                                                struct BinaryTreeNode*  node);
struct BinaryTreeNode* binaryTreeInternalRemove(struct BinaryTree*      bt,
                                                struct BinaryTreeNode** root,
                                                struct BinaryTreeNode*  node);
int binaryTreeIsEmpty(struct BinaryTree* bt);
void binaryTreePrint(struct BinaryTree* bt, FILE* fd);
struct BinaryTreeNode* binaryTreeGetFirst(const struct BinaryTree* bt);
struct BinaryTreeNode* binaryTreeGetLast(const struct BinaryTree* bt);
struct BinaryTreeNode* binaryTreeGetNearestPrev(
                                 struct BinaryTree*     bt,
                                 struct BinaryTreeNode* cmpNode);
struct BinaryTreeNode* binaryTreeGetNearestNext(
                                 struct BinaryTree*     bt,
                                 struct BinaryTreeNode* cmpNode);
size_t binaryTreeGetElements(const struct BinaryTree* bt);
struct BinaryTreeNode* binaryTreeGetPrev(const struct BinaryTree* bt,
                                         struct BinaryTreeNode*   cmpNode);
struct BinaryTreeNode* binaryTreeGetNext(const struct BinaryTree* bt,
                                         struct BinaryTreeNode*   cmpNode);
struct BinaryTreeNode* binaryTreeInsert(struct BinaryTree*     bt,
                                        struct BinaryTreeNode* node);
struct BinaryTreeNode* binaryTreeRemove(struct BinaryTree*     bt,
                                        struct BinaryTreeNode* node);
struct BinaryTreeNode* binaryTreeFind(const struct BinaryTree*     bt,
                                      const struct BinaryTreeNode* cmpNode);
BinaryTreeNodeValueType binaryTreeGetValueSum(const struct BinaryTree* bt);
struct BinaryTreeNode* binaryTreeGetNodeByValue(struct BinaryTree*      bt,
                                                BinaryTreeNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
