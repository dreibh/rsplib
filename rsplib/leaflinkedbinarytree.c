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

#include <stdlib.h>
#include <stdio.h>

#include "leaflinkedbinarytree.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ##### Initialize ###################################################### */
void leafLinkedBinaryTreeNew(struct LeafLinkedBinaryTree* llbt,
                             void                         (*printFunction)(const void* node, FILE* fd),
                             int                          (*comparisonFunction)(const void* node1, const void* node2))
{
   doubleLinkedRingListNew(&llbt->List);
   llbt->PrintFunction         = printFunction;
   llbt->ComparisonFunction    = comparisonFunction;
   llbt->NullNode.Parent       = &llbt->NullNode;
   llbt->NullNode.LeftSubtree  = &llbt->NullNode;
   llbt->NullNode.RightSubtree = &llbt->NullNode;
   llbt->NullNode.Value        = 0;
   llbt->NullNode.ValueSum     = 0;
   llbt->TreeRoot              = &llbt->NullNode;
   llbt->Elements              = 0;
}


/* ##### Invalidate ###################################################### */
void leafLinkedBinaryTreeDelete(struct LeafLinkedBinaryTree* llbt)
{
   llbt->Elements              = 0;
   llbt->TreeRoot              = NULL;
   llbt->NullNode.Parent       = NULL;
   llbt->NullNode.LeftSubtree  = NULL;
   llbt->NullNode.RightSubtree = NULL;
   doubleLinkedRingListDelete(&llbt->List);
}


/* ##### Update value sum ################################################ */
inline static void leafLinkedBinaryTreeUpdateValueSum(
                      struct LeafLinkedBinaryTreeNode* node)
{
   node->ValueSum = node->LeftSubtree->ValueSum + node->Value + node->RightSubtree->ValueSum;
}


/* ##### Update value sum of left and right sullbtree + current ones ####### */
inline static void leafLinkedBinaryTreeUpdateLeftAndRightValueSums(
                      struct LeafLinkedBinaryTreeNode* node)
{
   leafLinkedBinaryTreeUpdateValueSum(node->LeftSubtree);
   leafLinkedBinaryTreeUpdateValueSum(node->RightSubtree);
   leafLinkedBinaryTreeUpdateValueSum(node);
}


/* ##### Internal printing function ###################################### */
void leafLinkedBinaryTreeInternalPrint(struct LeafLinkedBinaryTree*     llbt,
                                       struct LeafLinkedBinaryTreeNode* node,
                                       FILE*                            fd)
{
   if(node != &llbt->NullNode) {
      leafLinkedBinaryTreeInternalPrint(llbt, node->LeftSubtree, fd);
      leafLinkedBinaryTreePrintNode(llbt, node, fd);
      leafLinkedBinaryTreeInternalPrint(llbt, node->RightSubtree, fd);
   }
}


/* ##### Internal insertion function ##################################### */
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalInsert(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  parent,
                                    struct LeafLinkedBinaryTreeNode*  node)
{
   struct LeafLinkedBinaryTreeNode* result;

   if(*root == &llbt->NullNode) {
      node->ListNode.Prev = NULL;
      node->ListNode.Next = NULL;
      node->Parent        = parent;
      node->LeftSubtree   = &llbt->NullNode;
      node->RightSubtree  = &llbt->NullNode;
      *root = node;
      llbt->Elements++;
      leafLinkedBinaryTreeUpdateValueSum(*root);
      result = node;
   }
   else {
      const int cmpResult = llbt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedBinaryTreeInternalInsert(llbt, &((*root)->LeftSubtree), *root, node);
         leafLinkedBinaryTreeUpdateValueSum(*root);
      }
      else if(cmpResult > 0) {
         result = leafLinkedBinaryTreeInternalInsert(llbt, &((*root)->RightSubtree), *root, node);
         leafLinkedBinaryTreeUpdateValueSum(*root);
      }
      else {
         /* Tried to insert duplicate -> return. */
         result = *root;
      }
   }
   return(result);
}


/* ##### Internal removal function ####################################### */
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalRemove(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  node)
{
   struct LeafLinkedBinaryTreeNode* result = NULL;
   struct LeafLinkedBinaryTreeNode* prev;

   if(*root != &llbt->NullNode) {
      const int cmpResult = llbt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedBinaryTreeInternalRemove(llbt, &((*root)->LeftSubtree), node);
         leafLinkedBinaryTreeUpdateValueSum(*root);
      }
      else if(cmpResult > 0) {
         result = leafLinkedBinaryTreeInternalRemove(llbt, &((*root)->RightSubtree), node);
         leafLinkedBinaryTreeUpdateValueSum(*root);
      }
      else {
         if(((*root)->LeftSubtree != &llbt->NullNode) &&
            ((*root)->RightSubtree !=&llbt->NullNode)) {
            prev = (*root)->LeftSubtree;
            while(prev->RightSubtree != &llbt->NullNode) {
               prev = prev->RightSubtree;
            }
            (*root)->RightSubtree->Parent = prev;
            prev->RightSubtree = (*root)->RightSubtree;
            (*root)->LeftSubtree->Parent = (*root)->Parent;
            *root = (*root)->LeftSubtree;
         }
         else {
            if((*root)->LeftSubtree != &llbt->NullNode) {
               (*root)->LeftSubtree->Parent = (*root)->Parent;
               *root = (*root)->LeftSubtree;
            }
            else if((*root)->RightSubtree != &llbt->NullNode) {
               (*root)->RightSubtree->Parent = (*root)->Parent;
               *root = (*root)->RightSubtree;
            }
            else {
               *root = &llbt->NullNode;
            }
         }

         node->Parent       = NULL;
         node->LeftSubtree  = NULL;
         node->RightSubtree = NULL;
         CHECK(llbt->Elements >= 1);
         llbt->Elements--;
         result = node;
      }
   }
   return(result);
}


/* ##### Get node by value ############################################### */
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeGetNodeByValue(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    LeafLinkedBinaryTreeNodeValueType value)
{
   struct LeafLinkedBinaryTreeNode* node = llbt->TreeRoot;
   for(;;) {
      if(value < node->LeftSubtree->ValueSum) {
         if(node->LeftSubtree != &llbt->NullNode) {
            node = node->LeftSubtree;
         }
         else {
            break;
         }
      }
      else if(value < node->LeftSubtree->ValueSum + node->Value) {
         break;
      }
      else {
         if(node->RightSubtree != &llbt->NullNode) {
            value -= node->LeftSubtree->ValueSum + node->Value;
            node = node->RightSubtree;
         }
         else {
            break;
         }
      }
   }

   if(node !=  &llbt->NullNode) {
      return(node);
   }
   return(NULL);
}


/* ##### Internal verification function ################################## */
static void leafLinkedBinaryTreeInternalVerify(
               struct LeafLinkedBinaryTree*      llbt,
               struct LeafLinkedBinaryTreeNode*  parent,
               struct LeafLinkedBinaryTreeNode*  node,
               struct LeafLinkedBinaryTreeNode** lastTreapNode,
               struct DoubleLinkedRingListNode** lastListNode,
               size_t*                           counter)
{
   struct LeafLinkedBinaryTreeNode* prev;
   struct LeafLinkedBinaryTreeNode* next;

   if(node != &llbt->NullNode) {
      /* ====== Print node =============================================== */
#ifdef DEBUG
      printf("verifying ");
      leafLinkedBinaryTreePrintNode(llbt, node);
      puts("");
#endif

      /* ====== Correct parent? ========================================== */
      CHECK(node->Parent == parent);

      /* ====== Correct tree and heap properties? ======================== */
      if(node->LeftSubtree != &llbt->NullNode) {
         CHECK(llbt->ComparisonFunction(node, node->LeftSubtree) > 0);
      }
      if(node->RightSubtree != &llbt->NullNode) {
         CHECK(llbt->ComparisonFunction(node, node->RightSubtree) < 0);
      }

      /* ====== Is value sum okay? ======================================= */
      CHECK(node->ValueSum == node->LeftSubtree->ValueSum +
                              node->Value +
                              node->RightSubtree->ValueSum);

      /* ====== Is left sullbtree okay? ==================================== */
      leafLinkedBinaryTreeInternalVerify(llbt, node, node->LeftSubtree, lastTreapNode, lastListNode, counter);

      /* ====== Is ring list okay? ======================================= */
      CHECK((*lastListNode)->Next != llbt->List.Head);
      *lastListNode = (*lastListNode)->Next;
      CHECK(*lastListNode == &node->ListNode);

      /* ====== Is linking working correctly? ============================ */
      prev = leafLinkedBinaryTreeInternalFindPrev(llbt, node);
      if(prev != &llbt->NullNode) {
         CHECK((*lastListNode)->Prev == &prev->ListNode);
      }
      else {
         CHECK((*lastListNode)->Prev == llbt->List.Head);
      }

      next = leafLinkedBinaryTreeInternalFindNext(llbt, node);
      if(next != &llbt->NullNode) {
         CHECK((*lastListNode)->Next == &next->ListNode);
      }
      else {
         CHECK((*lastListNode)->Next == llbt->List.Head);
      }

      /* ====== Count elements =========================================== */
      (*counter)++;

      /* ====== Is right sullbtree okay? =================================== */
      leafLinkedBinaryTreeInternalVerify(llbt, node, node->RightSubtree, lastTreapNode, lastListNode, counter);
   }
}


/* ##### Verify structures ############################################### */
void leafLinkedBinaryTreeVerify(struct LeafLinkedBinaryTree* llbt)
{
   size_t                           counter       = 0;
   struct LeafLinkedBinaryTreeNode* lastTreapNode = NULL;
   struct DoubleLinkedRingListNode* lastListNode  = &llbt->List.Node;

   /* leafLinkedBinaryTreePrint(llbt); */

   CHECK(llbt->NullNode.Parent       == &llbt->NullNode);
   CHECK(llbt->NullNode.LeftSubtree  == &llbt->NullNode);
   CHECK(llbt->NullNode.RightSubtree == &llbt->NullNode);
   CHECK(llbt->NullNode.Value        == 0);
   CHECK(llbt->NullNode.ValueSum     == 0);

   leafLinkedBinaryTreeInternalVerify(llbt, &llbt->NullNode, llbt->TreeRoot, &lastTreapNode, &lastListNode, &counter);
   CHECK(counter == llbt->Elements);
}


/* ##### Internal get nearest previous function ########################## */
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalGetNearestPrev(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  parent,
                                    struct LeafLinkedBinaryTreeNode*  node)
{
   struct LeafLinkedBinaryTreeNode* result;

   if(*root == &llbt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &llbt->NullNode;
      node->RightSubtree = &llbt->NullNode;
      *root = node;
      result = leafLinkedBinaryTreeInternalFindPrev(llbt, node);
      *root = &llbt->NullNode;
   }
   else {
      const int cmpResult = llbt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedBinaryTreeInternalGetNearestPrev(llbt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = leafLinkedBinaryTreeInternalGetNearestPrev(llbt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = leafLinkedBinaryTreeInternalFindPrev(llbt, *root);
      }
   }
   return(result);
}


/* ##### Internal get nearest next function ############################## */
struct LeafLinkedBinaryTreeNode* leafLinkedBinaryTreeInternalGetNearestNext(
                                    struct LeafLinkedBinaryTree*      llbt,
                                    struct LeafLinkedBinaryTreeNode** root,
                                    struct LeafLinkedBinaryTreeNode*  parent,
                                    struct LeafLinkedBinaryTreeNode*  node)
{
   struct LeafLinkedBinaryTreeNode* result;

   if(*root == &llbt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &llbt->NullNode;
      node->RightSubtree = &llbt->NullNode;
      *root = node;
      result = leafLinkedBinaryTreeInternalFindNext(llbt, node);
      *root = &llbt->NullNode;
   }
   else {
      const int cmpResult = llbt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedBinaryTreeInternalGetNearestNext(llbt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = leafLinkedBinaryTreeInternalGetNearestNext(llbt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = leafLinkedBinaryTreeInternalFindNext(llbt, *root);
      }
   }
   return(result);
}


#ifdef __cplusplus
}
#endif
