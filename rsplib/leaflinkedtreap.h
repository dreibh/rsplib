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

#ifndef LEAFLINKEDTREAP_H
#define LEAFLINKEDTREAP_H

#include <stdlib.h>
#include "debug.h"
#include "doublelinkedringlist.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned int       LeafLinkedTreapNodePriorityType;
typedef unsigned long long LeafLinkedTreapNodeValueType;


struct LeafLinkedTreapNode
{
   struct DoubleLinkedRingListNode ListNode;
   struct LeafLinkedTreapNode*     Parent;
   struct LeafLinkedTreapNode*     LeftSubtree;
   struct LeafLinkedTreapNode*     RightSubtree;
   LeafLinkedTreapNodePriorityType Priority;
   LeafLinkedTreapNodeValueType    Value;
   LeafLinkedTreapNodeValueType    ValueSum;  // ValueSum := LeftSubtree->Value + Value + RightSubtree->Value
};

struct LeafLinkedTreap
{
   struct LeafLinkedTreapNode* TreeRoot;
   struct LeafLinkedTreapNode  NullNode;
   struct DoubleLinkedRingList List;
   size_t                      Elements;
   void                        (*PrintFunction)(const void* node, FILE* fd);
   int                         (*ComparisonFunction)(const void* node1, const void* node2);
};


void leafLinkedTreapNodeNew(struct LeafLinkedTreapNode* node);
void leafLinkedTreapNodeDelete(struct LeafLinkedTreapNode* node);
int leafLinkedTreapNodeIsLinked(struct LeafLinkedTreapNode* node);


void leafLinkedTreapNew(struct LeafLinkedTreap* llt,
                        void                    (*printFunction)(const void* node, FILE* fd),
                        int                     (*comparisonFunction)(const void* node1, const void* node2));
void leafLinkedTreapDelete(struct LeafLinkedTreap* llt);
void leafLinkedTreapVerify(struct LeafLinkedTreap* llt);
void leafLinkedTreapInternalPrint(struct LeafLinkedTreap*     llt,
                                  struct LeafLinkedTreapNode* node,
                                  FILE*                       fd);
struct LeafLinkedTreapNode* leafLinkedTreapInternalGetNearestPrev(struct LeafLinkedTreap*      llt,
                                                                  struct LeafLinkedTreapNode** root,
                                                                  struct LeafLinkedTreapNode*  parent,
                                                                  struct LeafLinkedTreapNode*  node);
struct LeafLinkedTreapNode* leafLinkedTreapInternalGetNearestNext(struct LeafLinkedTreap*      llt,
                                                                  struct LeafLinkedTreapNode** root,
                                                                  struct LeafLinkedTreapNode*  parent,
                                                                  struct LeafLinkedTreapNode*  node);
struct LeafLinkedTreapNode* leafLinkedTreapInternalInsert(struct LeafLinkedTreap*      llt,
                                                          struct LeafLinkedTreapNode** root,
                                                          struct LeafLinkedTreapNode*  parent,
                                                          struct LeafLinkedTreapNode*  node);
struct LeafLinkedTreapNode* leafLinkedTreapInternalRemove(struct LeafLinkedTreap*      llt,
                                                          struct LeafLinkedTreapNode** root,
                                                          struct LeafLinkedTreapNode*  node);
int leafLinkedTreapIsEmpty(struct LeafLinkedTreap* llt);
void leafLinkedTreapPrint(struct LeafLinkedTreap* llt, FILE* fd);
struct LeafLinkedTreapNode* leafLinkedTreapGetFirst(const struct LeafLinkedTreap* llt);
struct LeafLinkedTreapNode* leafLinkedTreapGetLast(const struct LeafLinkedTreap* llt);
struct LeafLinkedTreapNode* leafLinkedTreapGetPrev(const struct LeafLinkedTreap* llt,
                                                   struct LeafLinkedTreapNode*   node);
struct LeafLinkedTreapNode* leafLinkedTreapGetNext(const struct LeafLinkedTreap* llt,
                                                   struct LeafLinkedTreapNode*   node);
struct LeafLinkedTreapNode* leafLinkedTreapGetNearestPrev(
                               struct LeafLinkedTreap*     llt,
                               struct LeafLinkedTreapNode* cmpNode);
struct LeafLinkedTreapNode* leafLinkedTreapGetNearestNext(
                               struct LeafLinkedTreap*     llt,
                               struct LeafLinkedTreapNode* cmpNode);
size_t leafLinkedTreapGetElements(const struct LeafLinkedTreap* llt);
struct LeafLinkedTreapNode* leafLinkedTreapInternalFindPrev(const struct LeafLinkedTreap* llt,
                                                            struct LeafLinkedTreapNode*   cmpNode);
struct LeafLinkedTreapNode* leafLinkedTreapInternalFindNext(const struct LeafLinkedTreap* llt,
                                                            struct LeafLinkedTreapNode*   cmpNode);
struct LeafLinkedTreapNode* leafLinkedTreapInsert(struct LeafLinkedTreap*     llt,
                                                  struct LeafLinkedTreapNode* node);
struct LeafLinkedTreapNode* leafLinkedTreapRemove(struct LeafLinkedTreap*     llt,
                                                  struct LeafLinkedTreapNode* node);
struct LeafLinkedTreapNode* leafLinkedTreapFind(const struct LeafLinkedTreap*     llt,
                                                const struct LeafLinkedTreapNode* cmpNode);
LeafLinkedTreapNodeValueType leafLinkedTreapGetValueSum(
                                       const struct LeafLinkedTreap* llt);
struct LeafLinkedTreapNode* leafLinkedTreapGetNodeByValue(
                               struct LeafLinkedTreap*      llt,
                               LeafLinkedTreapNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
