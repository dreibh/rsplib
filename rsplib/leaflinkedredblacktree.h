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

#include <stdlib.h>
#include "debug.h"
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


/* ###### Initialize ##################################################### */
inline void leafLinkedRedBlackTreeNodeNew(struct LeafLinkedRedBlackTreeNode* node)
{
   doubleLinkedRingListNodeNew(&node->ListNode);
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Color        = Black;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Invalidate ##################################################### */
inline void leafLinkedRedBlackTreeNodeDelete(struct LeafLinkedRedBlackTreeNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Color        = Black;
   node->Value        = 0;
   node->ValueSum     = 0;
   doubleLinkedRingListNodeDelete(&node->ListNode);
}


/* ###### Is node linked? ################################################ */
inline int leafLinkedRedBlackTreeNodeIsLinked(struct LeafLinkedRedBlackTreeNode* node)
{
   return(node->LeftSubtree != NULL);
}


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


/* ###### Is treap empty? ################################################ */
inline int leafLinkedRedBlackTreeIsEmpty(struct LeafLinkedRedBlackTree* llrbt)
{
   return(llrbt->NullNode.LeftSubtree == &llrbt->NullNode);
}


/* ###### Internal method for printing a node ############################# */
inline static void leafLinkedRedBlackTreePrintNode(struct LeafLinkedRedBlackTree*     llrbt,
                                                   struct LeafLinkedRedBlackTreeNode* node,
                                                   FILE*                              fd)
{
   llrbt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p c=%s v=%Lu vsum=%Lu",
           node, ((node->Color == Red) ? "Red" : "Black"),
           node->Value, node->ValueSum);
   if(node->LeftSubtree != &llrbt->NullNode) {
      fprintf(fd, " l=%p[", node->LeftSubtree);
      llrbt->PrintFunction(node->LeftSubtree, fd);
      fprintf(fd, "]");
   }
   else {
      fprintf(fd, " l=()");
   }
   if(node->RightSubtree != &llrbt->NullNode) {
      fprintf(fd, " r=%p[", node->RightSubtree);
      llrbt->PrintFunction(node->RightSubtree, fd);
      fprintf(fd, "]");
   }
   else {
      fprintf(fd, " r=()");
   }
   if(node->Parent != &llrbt->NullNode) {
      fprintf(fd, " p=%p[", node->Parent);
      llrbt->PrintFunction(node->Parent, fd);
      fprintf(fd, "]   ");
   }
   else {
      fprintf(fd, " p=())   ");
   }
   fputs("\n", fd);
#endif
}


/* ###### Print treap ##################################################### */
inline void leafLinkedRedBlackTreePrint(struct LeafLinkedRedBlackTree* llrbt,
                                        FILE*                          fd)
{
#ifdef DEBUG
   fprintf(fd, "\n\nroot=%p[", llrbt->NullNode.LeftSubtree);
   llrbt->PrintFunction(llrbt->NullNode.LeftSubtree);
   fprintf(fd, "] null=%p   \n", &llrbt->NullNode);
#endif
   leafLinkedRedBlackTreeInternalPrint(llrbt, llrbt->NullNode.LeftSubtree, fd);
   fputs("\n", fd);
}


/* ###### Get first node ################################################## */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetFirst(const struct LeafLinkedRedBlackTree* llrbt)
{
   struct DoubleLinkedRingListNode* node = llrbt->List.Node.Next;
   if(node != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)node);
   }
   return(NULL);
}


/* ###### Get last node ################################################### */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetLast(const struct LeafLinkedRedBlackTree* llrbt)
{
   struct DoubleLinkedRingListNode* node = llrbt->List.Node.Prev;
   if(node != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)node);
   }
   return(NULL);
}


/* ###### Get previous node ############################################### */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetPrev(const struct LeafLinkedRedBlackTree* llrbt,
                                                          struct LeafLinkedRedBlackTreeNode*   node)
{
   struct DoubleLinkedRingListNode* prev = node->ListNode.Prev;
   if(prev != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)prev);
   }
   return(NULL);
}


/* ###### Get next node ################################################## */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNext(const struct LeafLinkedRedBlackTree* llrbt,
                                                          struct LeafLinkedRedBlackTreeNode*   node)
{
   struct DoubleLinkedRingListNode* next = node->ListNode.Next;
   if(next != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)next);
   }
   return(NULL);
}


/* ###### Find nearest previous node ##################################### */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNearestPrev(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* cmpNode)
{
   struct LeafLinkedRedBlackTreeNode* result;
   result = leafLinkedRedBlackTreeInternalGetNearestPrev(
               llrbt, &llrbt->NullNode.LeftSubtree, &llrbt->NullNode, cmpNode);
   if(result != &llrbt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Find nearest next node ######################################### */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNearestNext(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* cmpNode)
{
   struct LeafLinkedRedBlackTreeNode* result;
   result = leafLinkedRedBlackTreeInternalGetNearestNext(
               llrbt, &llrbt->NullNode.LeftSubtree, &llrbt->NullNode, cmpNode);
   if(result != &llrbt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Get number of elements ########################################## */
inline size_t leafLinkedRedBlackTreeGetElements(const struct LeafLinkedRedBlackTree* llrbt)
{
   return(llrbt->Elements);
}


/* ###### Get prev node by walking through the tree (does *not* use list!) */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalFindPrev(
                                             const struct LeafLinkedRedBlackTree* llrbt,
                                             struct LeafLinkedRedBlackTreeNode*   cmpNode)
{
   struct LeafLinkedRedBlackTreeNode* node = cmpNode->LeftSubtree;
   struct LeafLinkedRedBlackTreeNode* parent;

   if(node != &llrbt->NullNode) {
      while(node->RightSubtree != &llrbt->NullNode) {
         node = node->RightSubtree;
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &llrbt->NullNode) && (node == parent->LeftSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return(parent);
   }
}


/* ###### Get next node by walking through the tree (does *not* use list!) */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalFindNext(
                                             const struct LeafLinkedRedBlackTree* llrbt,
                                             struct LeafLinkedRedBlackTreeNode*   cmpNode)
{
   struct LeafLinkedRedBlackTreeNode* node = cmpNode->RightSubtree;
   struct LeafLinkedRedBlackTreeNode* parent;

   if(node != &llrbt->NullNode) {
      while(node->LeftSubtree != &llrbt->NullNode) {
         node = node->LeftSubtree;
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &llrbt->NullNode) && (node == parent->RightSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return(parent);
   }
}


struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInsert(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* node);
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeRemove(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* node);


/* ###### Find node ####################################################### */
inline struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeFind(
                                             const struct LeafLinkedRedBlackTree*     llrbt,
                                             const struct LeafLinkedRedBlackTreeNode* cmpNode)
{
#ifdef DEBUG
   printf("find: ");
   llrbt->PrintFunction(cmpNode);
   printf("\n");
#endif

   struct LeafLinkedRedBlackTreeNode* node = llrbt->NullNode.LeftSubtree;
   while(node != &llrbt->NullNode) {
      const int cmpResult = llrbt->ComparisonFunction(cmpNode, node);
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
inline LeafLinkedRedBlackTreeNodeValueType leafLinkedRedBlackTreeGetValueSum(
                                              const struct LeafLinkedRedBlackTree* llrbt)
{
   return(llrbt->NullNode.LeftSubtree->ValueSum);
}


struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNodeByValue(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      LeafLinkedRedBlackTreeNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
