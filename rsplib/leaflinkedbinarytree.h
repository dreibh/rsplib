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
   LeafLinkedBinaryTreeNodeValueType ValueSum;  // ValueSum := LeftSubtree->Value + Value + RightSubtree->Value
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


/* ###### Initialize ##################################################### */
inline void leafLinkedBinaryTreeNodeNew(struct LeafLinkedBinaryTreeNode* node)
{
   doubleLinkedRingListNodeNew(&node->ListNode);
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Invalidate ##################################################### */
inline void leafLinkedBinaryTreeNodeDelete(struct LeafLinkedBinaryTreeNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Value        = 0;
   node->ValueSum     = 0;
   doubleLinkedRingListNodeDelete(&node->ListNode);
}


/* ###### Is node linked? ################################################ */
inline int leafLinkedBinaryTreeNodeIsLinked(struct LeafLinkedBinaryTreeNode* node)
{
   return(node->LeftSubtree != NULL);
}


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


/* ###### Is treap empty? ################################################ */
inline int leafLinkedBinaryTreeIsEmpty(struct LeafLinkedBinaryTree* llbt)
{
   return(llbt->TreeRoot == &llbt->NullNode);
}


/* ###### Internal method for printing a node ############################# */
inline static void leafLinkedBinaryTreePrintNode(struct LeafLinkedBinaryTree*     llbt,
                                                 struct LeafLinkedBinaryTreeNode* node,
                                                 FILE*                            fd)
{
   llbt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p v=%Lu vsum=%Lu", node, node->Value, node->ValueSum);
   if(node->LeftSubtree != &llbt->NullNode) {
      fprintf(fd, " l=%p", node->LeftSubtree);
   }
   else {
      fprintf(fd, " l=()");
   }
   if(node->RightSubtree != &llbt->NullNode) {
      fprintf(fd, " r=%p", node->RightSubtree);
   }
   else {
      fprintf(fd, " r=()");
   }
   if(node->Parent != &llbt->NullNode) {
      fprintf(fd, " p=%p ", node->Parent);
   }
   else {
      fprintf(fd, " p=())   ");
   }
#endif
}


/* ###### Print treap ##################################################### */
inline void leafLinkedBinaryTreePrint(struct LeafLinkedBinaryTree* llbt,
                                      FILE*                        fd)
{
#ifdef DEBUG
   printf("root=%p null=%p   ", llbt->TreeRoot, &llbt->NullNode);
#endif
   leafLinkedBinaryTreeInternalPrint(llbt, llbt->TreeRoot, fd);
   puts("");
}


/* ###### Get first node ################################################## */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetFirst(const struct LeafLinkedBinaryTree* llbt)
{
   struct DoubleLinkedRingListNode* node = llbt->List.Node.Next;
   if(node != llbt->List.Head) {
      return((struct LeafLinkedBinaryTreeNode*)node);
   }
   return(NULL);
}


/* ###### Get last node ################################################### */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetLast(const struct LeafLinkedBinaryTree* llbt)
{
   struct DoubleLinkedRingListNode* node = llbt->List.Node.Prev;
   if(node != llbt->List.Head) {
      return((struct LeafLinkedBinaryTreeNode*)node);
   }
   return(NULL);
}


/* ###### Get previous node ############################################### */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetPrev(const struct LeafLinkedBinaryTree* llbt,
                                                                    struct LeafLinkedBinaryTreeNode*   node)
{
   struct DoubleLinkedRingListNode* prev = node->ListNode.Prev;
   if(prev != llbt->List.Head) {
      return((struct LeafLinkedBinaryTreeNode*)prev);
   }
   return(NULL);
}


/* ###### Get next node ################################################## */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNext(const struct LeafLinkedBinaryTree* llbt,
                                                                    struct LeafLinkedBinaryTreeNode*   node)
{
   struct DoubleLinkedRingListNode* next = node->ListNode.Next;
   if(next != llbt->List.Head) {
      return((struct LeafLinkedBinaryTreeNode*)next);
   }
   return(NULL);
}


/* ###### Find nearest previous node ##################################### */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNearestPrev(
                                           struct LeafLinkedBinaryTree*     llbt,
                                           struct LeafLinkedBinaryTreeNode* cmpNode)
{
   struct LeafLinkedBinaryTreeNode* result;
   result = leafLinkedBinaryTreeInternalGetNearestPrev(
               llbt, &llbt->TreeRoot, &llbt->NullNode, cmpNode);
   if(result != &llbt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Find nearest next node ######################################### */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNearestNext(
                                           struct LeafLinkedBinaryTree*     llbt,
                                           struct LeafLinkedBinaryTreeNode* cmpNode)
{
   struct LeafLinkedBinaryTreeNode* result;
   result = leafLinkedBinaryTreeInternalGetNearestNext(
               llbt, &llbt->TreeRoot, &llbt->NullNode, cmpNode);
   if(result != &llbt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Get number of elements ########################################## */
inline size_t leafLinkedBinaryTreeGetElements(const struct LeafLinkedBinaryTree* llbt)
{
   return(llbt->Elements);
}


/* ###### Get prev node by walking through the tree (does *not* use list!) */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalFindPrev(const struct LeafLinkedBinaryTree* llbt,
                                                                             struct LeafLinkedBinaryTreeNode*   cmpNode)
{
   struct LeafLinkedBinaryTreeNode* node = cmpNode->LeftSubtree;
   struct LeafLinkedBinaryTreeNode* parent;

   if(node != &llbt->NullNode) {
      while(node->RightSubtree != &llbt->NullNode) {
         node = node->RightSubtree;
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &llbt->NullNode) && (node == parent->LeftSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return(parent);
   }
}


/* ###### Get next node by walking through the tree (does *not* use list!) */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalFindNext(const struct LeafLinkedBinaryTree* llbt,
                                                                             struct LeafLinkedBinaryTreeNode*   cmpNode)
{
   struct LeafLinkedBinaryTreeNode* node = cmpNode->RightSubtree;
   struct LeafLinkedBinaryTreeNode* parent;

   if(node != &llbt->NullNode) {
      while(node->LeftSubtree != &llbt->NullNode) {
         node = node->LeftSubtree;
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &llbt->NullNode) && (node == parent->RightSubtree)) {
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
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInsert(struct LeafLinkedBinaryTree*     llbt,
                                                                   struct LeafLinkedBinaryTreeNode* node)
{
   struct LeafLinkedBinaryTreeNode* result;
   struct LeafLinkedBinaryTreeNode* prev;
#ifdef DEBUG
   printf("insert: ");
   llbt->PrintFunction(node);
   printf("\n");
#endif

   result = leafLinkedBinaryTreeInternalInsert(llbt, &llbt->TreeRoot, &llbt->NullNode, node);
   if(result == node) {
      // Important: The NullNode's parent pointer may be modified during rotations.
      // We reset it here. This is much more efficient than if-clauses in the
      // rotation functions.
      llbt->NullNode.Parent = &llbt->NullNode;

      prev = leafLinkedBinaryTreeInternalFindPrev(llbt, node);
      if(prev != &llbt->NullNode) {
         doubleLinkedRingListAddAfter(&prev->ListNode, &node->ListNode);
      }
      else {
         doubleLinkedRingListAddHead(&llbt->List, &node->ListNode);
      }
#ifdef DEBUG
      leafLinkedBinaryTreePrint(llbt, stdout);
#endif
#ifdef VERIFY
      leafLinkedBinaryTreeVerify(llbt);
#endif
   }
   return(result);
}


/* ###### Remove node ##################################################### */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeRemove(struct LeafLinkedBinaryTree*     llbt,
                                                                   struct LeafLinkedBinaryTreeNode* node)
{
   struct LeafLinkedBinaryTreeNode* result;
#ifdef DEBUG
   printf("remove: ");
   llbt->PrintFunction(node);
   printf("\n");
#endif

   result = leafLinkedBinaryTreeInternalRemove(llbt, &llbt->TreeRoot, node);
   if(result) {
      // Important: The NullNode's parent pointer may be modified during rotations.
      // We reset it here. This is much more efficient than if-clauses in the
      // rotation functions.
      llbt->NullNode.Parent = &llbt->NullNode;

      doubleLinkedRingListRemNode(&node->ListNode);
      node->ListNode.Prev = NULL;
      node->ListNode.Next = NULL;

#ifdef DEBUG
      leafLinkedBinaryTreePrint(llbt, stdout);
#endif
#ifdef VERIFY
      leafLinkedBinaryTreeVerify(llbt);
#endif
   }
   return(result);
}


/* ###### Find node ####################################################### */
inline struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeFind(const struct LeafLinkedBinaryTree*     llbt,
                                                                 const struct LeafLinkedBinaryTreeNode* cmpNode)
{
#ifdef DEBUG
   printf("find: ");
   llbt->PrintFunction(cmpNode);
   printf("\n");
#endif

   struct LeafLinkedBinaryTreeNode* node = llbt->TreeRoot;
   while(node != &llbt->NullNode) {
      const int cmpResult = llbt->ComparisonFunction(cmpNode, node);
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
inline LeafLinkedBinaryTreeNodeValueType leafLinkedBinaryTreeGetValueSum(
                                            const struct LeafLinkedBinaryTree* llbt)
{
   return(llbt->TreeRoot->ValueSum);
}


struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNodeByValue(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    LeafLinkedBinaryTreeNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
