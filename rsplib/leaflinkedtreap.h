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


/* ###### Initialize ##################################################### */
inline void leafLinkedTreapNodeNew(struct LeafLinkedTreapNode* node)
{
   doubleLinkedRingListNodeNew(&node->ListNode);
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Priority     = 0;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Invalidate ##################################################### */
inline void leafLinkedTreapNodeDelete(struct LeafLinkedTreapNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Priority     = 0;
   node->Value        = 0;
   node->ValueSum     = 0;
   doubleLinkedRingListNodeDelete(&node->ListNode);
}


/* ###### Is node linked? ################################################ */
inline int leafLinkedTreapNodeIsLinked(struct LeafLinkedTreapNode* node)
{
   return(node->LeftSubtree != NULL);
}


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


/* ###### Is treap empty? ################################################ */
inline int leafLinkedTreapIsEmpty(struct LeafLinkedTreap* llt)
{
   return(llt->TreeRoot == &llt->NullNode);
}


/* ###### Internal method for printing a node ############################# */
inline static void leafLinkedTreapPrintNode(struct LeafLinkedTreap*     llt,
                                            struct LeafLinkedTreapNode* node,
                                            FILE*                       fd)
{
   llt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p v=%Lu vsum=%Lu", node, node->Value, node->ValueSum);
   if(node->LeftSubtree != &llt->NullNode) {
      fprintf(fd, " l=%p", node->LeftSubtree);
   }
   else {
      fprintf(fd, " l=()");
   }
   if(node->RightSubtree != &llt->NullNode) {
      fprintf(fd, " r=%p", node->RightSubtree);
   }
   else {
      fprintf(fd, " r=()");
   }
   if(node->Parent != &llt->NullNode) {
      fprintf(fd, " p=%p ", node->Parent);
   }
   else {
      fprintf(fd, " p=())   ");
   }
#endif
}


/* ###### Print treap ##################################################### */
inline void leafLinkedTreapPrint(struct LeafLinkedTreap* llt, FILE* fd)
{
#ifdef DEBUG
   fprintf(fd, "root=%p null=%p   ", llt->TreeRoot, &llt->NullNode);
#endif
   leafLinkedTreapInternalPrint(llt, llt->TreeRoot, fd);
   fputs("\n", fd);
}


/* ###### Get first node ################################################## */
inline struct LeafLinkedTreapNode* leafLinkedTreapGetFirst(const struct LeafLinkedTreap* llt)
{
   struct DoubleLinkedRingListNode* node = llt->List.Node.Next;
   if(node != llt->List.Head) {
      return((struct LeafLinkedTreapNode*)node);
   }
   return(NULL);
}


/* ###### Get last node ################################################### */
inline struct LeafLinkedTreapNode* leafLinkedTreapGetLast(const struct LeafLinkedTreap* llt)
{
   struct DoubleLinkedRingListNode* node = llt->List.Node.Prev;
   if(node != llt->List.Head) {
      return((struct LeafLinkedTreapNode*)node);
   }
   return(NULL);
}


/* ###### Get previous node ############################################### */
inline struct LeafLinkedTreapNode* leafLinkedTreapGetPrev(const struct LeafLinkedTreap* llt,
                                                          struct LeafLinkedTreapNode*   node)
{
   struct DoubleLinkedRingListNode* prev = node->ListNode.Prev;
   if(prev != llt->List.Head) {
      return((struct LeafLinkedTreapNode*)prev);
   }
   return(NULL);
}


/* ###### Get next node ################################################## */
inline struct LeafLinkedTreapNode* leafLinkedTreapGetNext(const struct LeafLinkedTreap* llt,
                                                          struct LeafLinkedTreapNode*   node)
{
   struct DoubleLinkedRingListNode* next = node->ListNode.Next;
   if(next != llt->List.Head) {
      return((struct LeafLinkedTreapNode*)next);
   }
   return(NULL);
}


/* ###### Find nearest previous node ##################################### */
inline struct LeafLinkedTreapNode* leafLinkedTreapGetNearestPrev(
                                      struct LeafLinkedTreap*     llt,
                                      struct LeafLinkedTreapNode* cmpNode)
{
   struct LeafLinkedTreapNode* result;
   result = leafLinkedTreapInternalGetNearestPrev(
               llt, &llt->TreeRoot, &llt->NullNode, cmpNode);
   if(result != &llt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Find nearest next node ######################################### */
inline struct LeafLinkedTreapNode* leafLinkedTreapGetNearestNext(
                                      struct LeafLinkedTreap*     llt,
                                      struct LeafLinkedTreapNode* cmpNode)
{
   struct LeafLinkedTreapNode* result;
   result = leafLinkedTreapInternalGetNearestNext(
               llt, &llt->TreeRoot, &llt->NullNode, cmpNode);
   if(result != &llt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Get number of elements ########################################## */
inline size_t leafLinkedTreapGetElements(const struct LeafLinkedTreap* llt)
{
   return(llt->Elements);
}


/* ###### Get prev node by walking through the tree (does *not* use list!) */
inline struct LeafLinkedTreapNode* leafLinkedTreapInternalFindPrev(const struct LeafLinkedTreap* llt,
                                                                   struct LeafLinkedTreapNode*   cmpNode)
{
   struct LeafLinkedTreapNode* node = cmpNode->LeftSubtree;
   struct LeafLinkedTreapNode* parent;

   if(node != &llt->NullNode) {
      while(node->RightSubtree != &llt->NullNode) {
         node = node->RightSubtree;
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &llt->NullNode) && (node == parent->LeftSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return(parent);
   }
}


/* ###### Get next node by walking through the tree (does *not* use list!) */
inline struct LeafLinkedTreapNode* leafLinkedTreapInternalFindNext(const struct LeafLinkedTreap* llt,
                                                                   struct LeafLinkedTreapNode*   cmpNode)
{
   struct LeafLinkedTreapNode* node = cmpNode->RightSubtree;
   struct LeafLinkedTreapNode* parent;

   if(node != &llt->NullNode) {
      while(node->LeftSubtree != &llt->NullNode) {
         node = node->LeftSubtree;
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &llt->NullNode) && (node == parent->RightSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return(parent);
   }
}


/* ###### Insert node ##################################################### */
/*
   returns node, if node has been inserted. Otherwise, duplicate node
   already in treap is returned.
*/
inline struct LeafLinkedTreapNode* leafLinkedTreapInsert(struct LeafLinkedTreap*     llt,
                                                         struct LeafLinkedTreapNode* node)
{
   struct LeafLinkedTreapNode* result;
   struct LeafLinkedTreapNode* prev;
#ifdef DEBUG
   printf("insert: ");
   llt->PrintFunction(node);
   printf("\n");
#endif

   result = leafLinkedTreapInternalInsert(llt, &llt->TreeRoot, &llt->NullNode, node);
   if(result == node) {
      // Important: The NullNode's parent pointer may be modified during rotations.
      // We reset it here. This is much more efficient than if-clauses in the
      // rotation functions.
      llt->NullNode.Parent = &llt->NullNode;

      prev = leafLinkedTreapInternalFindPrev(llt, node);
      if(prev != &llt->NullNode) {
         doubleLinkedRingListAddAfter(&prev->ListNode, &node->ListNode);
      }
      else {
         doubleLinkedRingListAddHead(&llt->List, &node->ListNode);
      }
#ifdef DEBUG
      leafLinkedTreapPrint(llt);
#endif
#ifdef VERIFY
      leafLinkedTreapVerify(llt);
#endif
   }
   return(result);
}


/* ###### Remove node ##################################################### */
inline struct LeafLinkedTreapNode* leafLinkedTreapRemove(struct LeafLinkedTreap*     llt,
                                                         struct LeafLinkedTreapNode* node)
{
   struct LeafLinkedTreapNode* result;
#ifdef DEBUG
   printf("remove: ");
   llt->PrintFunction(node);
   printf("\n");
#endif

   result = leafLinkedTreapInternalRemove(llt, &llt->TreeRoot, node);
   if(result) {
      // Important: The NullNode's parent pointer may be modified during rotations.
      // We reset it here. This is much more efficient than if-clauses in the
      // rotation functions.
      llt->NullNode.Parent = &llt->NullNode;

      doubleLinkedRingListRemNode(&node->ListNode);
      node->ListNode.Prev = NULL;
      node->ListNode.Next = NULL;

#ifdef DEBUG
      leafLinkedTreapPrint(llt);
#endif
#ifdef VERIFY
      leafLinkedTreapVerify(llt);
#endif
   }
   return(result);
}


/* ###### Find node ####################################################### */
inline struct LeafLinkedTreapNode* leafLinkedTreapFind(const struct LeafLinkedTreap*     llt,
                                                       const struct LeafLinkedTreapNode* cmpNode)
{
#ifdef DEBUG
   printf("find: ");
   llt->PrintFunction(cmpNode);
   printf("\n");
#endif

   struct LeafLinkedTreapNode* node = llt->TreeRoot;
   while(node != &llt->NullNode) {
      const int cmpResult = llt->ComparisonFunction(cmpNode, node);
      if(cmpResult == 0) {
         return(node);
      }
      else if(cmpResult < 0) {
         node = node->LeftSubtree;
      }
      else {
         node = node->RightSubtree;
      }
   }
   return(NULL);
}


/* ###### Get value sum from root node ################################### */
inline LeafLinkedTreapNodeValueType leafLinkedTreapGetValueSum(
                                       const struct LeafLinkedTreap* llt)
{
   return(llt->TreeRoot->ValueSum);
}


struct LeafLinkedTreapNode* leafLinkedTreapGetNodeByValue(
                               struct LeafLinkedTreap*      llt,
                               LeafLinkedTreapNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
