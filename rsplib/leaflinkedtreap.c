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

#include "leaflinkedtreap.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ##### Initialize ###################################################### */
void leafLinkedTreapNew(struct LeafLinkedTreap* llt,
                        void                    (*printFunction)(const void* node, FILE* fd),
                        int                     (*comparisonFunction)(const void* node1, const void* node2))
{
   doubleLinkedRingListNew(&llt->List);
   llt->PrintFunction         = printFunction;
   llt->ComparisonFunction    = comparisonFunction;
   llt->NullNode.Parent       = &llt->NullNode;
   llt->NullNode.LeftSubtree  = &llt->NullNode;
   llt->NullNode.RightSubtree = &llt->NullNode;
   llt->NullNode.Priority     = ~0;
   llt->NullNode.Value        = 0;
   llt->NullNode.ValueSum     = 0;
   llt->TreeRoot              = &llt->NullNode;
   llt->Elements              = 0;
}


/* ##### Invalidate ###################################################### */
void leafLinkedTreapDelete(struct LeafLinkedTreap* llt)
{
   llt->Elements = 0;
   llt->TreeRoot              = NULL;
   llt->NullNode.Parent       = NULL;
   llt->NullNode.LeftSubtree  = NULL;
   llt->NullNode.RightSubtree = NULL;
   doubleLinkedRingListDelete(&llt->List);
}


/* ##### Update value sum ################################################ */
inline static void leafLinkedTreapUpdateValueSum(struct LeafLinkedTreapNode* node)
{
   node->ValueSum = node->LeftSubtree->ValueSum + node->Value + node->RightSubtree->ValueSum;
}


/* ##### Update value sum of left and right subtree + current ones ####### */
inline static void leafLinkedTreapUpdateLeftAndRightValueSums(
                      struct LeafLinkedTreapNode* node)
{
   leafLinkedTreapUpdateValueSum(node->LeftSubtree);
   leafLinkedTreapUpdateValueSum(node->RightSubtree);
   leafLinkedTreapUpdateValueSum(node);
}


/* ##### Internal printing function ###################################### */
void leafLinkedTreapInternalPrint(struct LeafLinkedTreap*     llt,
                                  struct LeafLinkedTreapNode* node,
                                  FILE*                       fd)
{
   if(node != &llt->NullNode) {
      leafLinkedTreapInternalPrint(llt, node->LeftSubtree, fd);
      leafLinkedTreapPrintNode(llt, node, fd);
      leafLinkedTreapInternalPrint(llt, node->RightSubtree, fd);
   }
}




/* ##### Rotation with left subtree ###################################### */
inline static void leafLinkedTreapRotateWithLeftSubtree(
                      struct LeafLinkedTreapNode** node2)
{
   struct LeafLinkedTreapNode* parent = (*node2)->Parent;
   struct LeafLinkedTreapNode* node1 = (*node2)->LeftSubtree;
   (*node2)->LeftSubtree = node1->RightSubtree;
   node1->RightSubtree   = *node2;
   *node2                = node1;

   (*node2)->Parent                             = parent;
   (*node2)->LeftSubtree->Parent                = *node2;
   (*node2)->RightSubtree->Parent               = *node2;
   (*node2)->RightSubtree->LeftSubtree->Parent  = (*node2)->RightSubtree;
   (*node2)->RightSubtree->RightSubtree->Parent = (*node2)->RightSubtree;
}


/* ##### Rotation with ripht subtree ##################################### */
inline static void leafLinkedTreapRotateWithRightSubtree(
                      struct LeafLinkedTreapNode** node1)
{
   struct LeafLinkedTreapNode* parent = (*node1)->Parent;
   struct LeafLinkedTreapNode* node2 = (*node1)->RightSubtree;
   (*node1)->RightSubtree = node2->LeftSubtree;
   node2->LeftSubtree     = *node1;
   *node1                 = node2;

   (*node1)->Parent                            = parent;
   (*node1)->LeftSubtree->Parent               = *node1;
   (*node1)->RightSubtree->Parent              = *node1;
   (*node1)->LeftSubtree->LeftSubtree->Parent  = (*node1)->LeftSubtree;
   (*node1)->LeftSubtree->RightSubtree->Parent = (*node1)->LeftSubtree;
}


/* ##### Internal insertion function ##################################### */
struct LeafLinkedTreapNode* leafLinkedTreapInternalInsert(
                               struct LeafLinkedTreap*      llt,
                               struct LeafLinkedTreapNode** root,
                               struct LeafLinkedTreapNode*  parent,
                               struct LeafLinkedTreapNode*  node)
{
   struct LeafLinkedTreapNode* result;

   if(*root == &llt->NullNode) {
      node->ListNode.Prev = NULL;
      node->ListNode.Next = NULL;
      node->Parent       = parent;
      node->LeftSubtree  = &llt->NullNode;
      node->RightSubtree = &llt->NullNode;
      node->Priority     = (unsigned int)random();
      *root = node;
      llt->Elements++;
      leafLinkedTreapUpdateValueSum(*root);
      result = node;
   }
   else {
      const int cmpResult = llt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedTreapInternalInsert(llt, &((*root)->LeftSubtree), *root, node);
         leafLinkedTreapUpdateValueSum(*root);
         if((*root)->LeftSubtree->Priority < (*root)->Priority) {
            leafLinkedTreapRotateWithLeftSubtree(root);
            leafLinkedTreapUpdateLeftAndRightValueSums(*root);
         }
      }
      else if(cmpResult > 0) {
         result = leafLinkedTreapInternalInsert(llt, &((*root)->RightSubtree), *root, node);
         leafLinkedTreapUpdateValueSum(*root);
         if((*root)->RightSubtree->Priority < (*root)->Priority) {
            leafLinkedTreapRotateWithRightSubtree(root);
            leafLinkedTreapUpdateLeftAndRightValueSums(*root);
         }
      }
      else {
         /* Tried to insert duplicate -> return. */
         result = *root;
      }
   }
   return(result);
}


/* ##### Internal removal function ####################################### */
struct LeafLinkedTreapNode* leafLinkedTreapInternalRemove(
                               struct LeafLinkedTreap*      llt,
                               struct LeafLinkedTreapNode** root,
                               struct LeafLinkedTreapNode*  node)
{
   struct LeafLinkedTreapNode* result = NULL;

   if(*root != &llt->NullNode) {
      const int cmpResult = llt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedTreapInternalRemove(llt, &((*root)->LeftSubtree), node);
         leafLinkedTreapUpdateValueSum(*root);
      }
      else if(cmpResult > 0) {
         result = leafLinkedTreapInternalRemove(llt, &((*root)->RightSubtree), node);
         leafLinkedTreapUpdateValueSum(*root);
      }
      else {
         if((*root)->LeftSubtree->Priority < (*root)->RightSubtree->Priority) {
            leafLinkedTreapRotateWithLeftSubtree(root);
         }
         else {
            leafLinkedTreapRotateWithRightSubtree(root);
         }

         if(*root != &llt->NullNode) {
            result = leafLinkedTreapInternalRemove(llt, root, node);
         }
         else {
            (*root)->LeftSubtree->Parent       = NULL;
            (*root)->LeftSubtree->LeftSubtree  = NULL;
            (*root)->LeftSubtree->RightSubtree = NULL;
            (*root)->LeftSubtree = &llt->NullNode;
            CHECK(llt->Elements >= 1);
            llt->Elements--;
            result = node;
         }
      }
   }
   return(result);
}


/* ##### Get node by value ############################################### */
struct LeafLinkedTreapNode* leafLinkedTreapGetNodeByValue(
                               struct LeafLinkedTreap*      llt,
                               LeafLinkedTreapNodeValueType value)
{
   struct LeafLinkedTreapNode* node = llt->TreeRoot;
   for(;;) {
      if(value < node->LeftSubtree->ValueSum) {
         if(node->LeftSubtree != &llt->NullNode) {
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
         if(node->RightSubtree != &llt->NullNode) {
            value -= node->LeftSubtree->ValueSum + node->Value;
            node = node->RightSubtree;
         }
         else {
            break;
         }
      }
   }

   if(node !=  &llt->NullNode) {
      return(node);
   }
   return(NULL);
}


/* ##### Internal verification function ################################## */
static void leafLinkedTreapInternalVerify(struct LeafLinkedTreap*           llt,
                                          struct LeafLinkedTreapNode*       parent,
                                          struct LeafLinkedTreapNode*       node,
                                          struct LeafLinkedTreapNode**      lastTreapNode,
                                          struct DoubleLinkedRingListNode** lastListNode,
                                          size_t*                           counter)
{
   struct LeafLinkedTreapNode* prev;
   struct LeafLinkedTreapNode* next;

   if(node != &llt->NullNode) {
      /* ====== Print node =============================================== */
#ifdef DEBUG
      printf("verifying ");
      leafLinkedTreapPrintNode(llt, node);
      puts("");
#endif

      /* ====== Correct parent? ========================================== */
      CHECK(node->Parent == parent);

      /* ====== Correct tree and heap properties? ======================== */
      if(node->LeftSubtree != &llt->NullNode) {
         CHECK(llt->ComparisonFunction(node, node->LeftSubtree) > 0);
         CHECK(node->Priority < node->LeftSubtree->Priority);
      }
      if(node->RightSubtree != &llt->NullNode) {
         CHECK(llt->ComparisonFunction(node, node->RightSubtree) < 0);
         CHECK(node->Priority < node->RightSubtree->Priority);
      }

      /* ====== Is value sum okay? ======================================= */
      CHECK(node->ValueSum == node->LeftSubtree->ValueSum +
                              node->Value +
                              node->RightSubtree->ValueSum);

      /* ====== Is left subtree okay? ==================================== */
      leafLinkedTreapInternalVerify(llt, node, node->LeftSubtree, lastTreapNode, lastListNode, counter);

      /* ====== Is ring list okay? ======================================= */
      CHECK((*lastListNode)->Next != llt->List.Head);
      *lastListNode = (*lastListNode)->Next;
      CHECK(*lastListNode == &node->ListNode);

      /* ====== Is linking working correctly? ============================ */
      prev = leafLinkedTreapInternalFindPrev(llt, node);
      if(prev != &llt->NullNode) {
         CHECK((*lastListNode)->Prev == &prev->ListNode);
      }
      else {
         CHECK((*lastListNode)->Prev == llt->List.Head);
      }

      next = leafLinkedTreapInternalFindNext(llt, node);
      if(next != &llt->NullNode) {
         CHECK((*lastListNode)->Next == &next->ListNode);
      }
      else {
         CHECK((*lastListNode)->Next == llt->List.Head);
      }

      /* ====== Count elements =========================================== */
      (*counter)++;

      /* ====== Is right subtree okay? =================================== */
      leafLinkedTreapInternalVerify(llt, node, node->RightSubtree, lastTreapNode, lastListNode, counter);
   }
}


/* ##### Verify structures ############################################### */
void leafLinkedTreapVerify(struct LeafLinkedTreap* llt)
{
   size_t                           counter       = 0;
   struct LeafLinkedTreapNode*        lastTreapNode = NULL;
   struct DoubleLinkedRingListNode* lastListNode  = &llt->List.Node;

   /* leafLinkedTreapPrint(llt); */

   CHECK(llt->NullNode.Parent       == &llt->NullNode);
   CHECK(llt->NullNode.LeftSubtree  == &llt->NullNode);
   CHECK(llt->NullNode.RightSubtree == &llt->NullNode);
   CHECK(llt->NullNode.Value == 0);
   CHECK(llt->NullNode.ValueSum == 0);

   leafLinkedTreapInternalVerify(llt, &llt->NullNode, llt->TreeRoot, &lastTreapNode, &lastListNode, &counter);
   CHECK(counter == llt->Elements);
}


/* ##### Internal get nearest previous function ########################## */
struct LeafLinkedTreapNode* leafLinkedTreapInternalGetNearestPrev(
                               struct LeafLinkedTreap*      llt,
                               struct LeafLinkedTreapNode** root,
                               struct LeafLinkedTreapNode*  parent,
                               struct LeafLinkedTreapNode*  node)
{
   struct LeafLinkedTreapNode* result;

   if(*root == &llt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &llt->NullNode;
      node->RightSubtree = &llt->NullNode;
      *root = node;
      result = leafLinkedTreapInternalFindPrev(llt, node);
      *root = &llt->NullNode;
   }
   else {
      const int cmpResult = llt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedTreapInternalGetNearestPrev(llt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = leafLinkedTreapInternalGetNearestPrev(llt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = leafLinkedTreapInternalFindPrev(llt, *root);
      }
   }
   return(result);
}


/* ##### Internal get nearest next function ############################## */
struct LeafLinkedTreapNode* leafLinkedTreapInternalGetNearestNext(
                               struct LeafLinkedTreap*      llt,
                               struct LeafLinkedTreapNode** root,
                               struct LeafLinkedTreapNode*  parent,
                               struct LeafLinkedTreapNode*  node)
{
   struct LeafLinkedTreapNode* result;

   if(*root == &llt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &llt->NullNode;
      node->RightSubtree = &llt->NullNode;
      *root = node;
      result = leafLinkedTreapInternalFindNext(llt, node);
      *root = &llt->NullNode;
   }
   else {
      const int cmpResult = llt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedTreapInternalGetNearestNext(llt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = leafLinkedTreapInternalGetNearestNext(llt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = leafLinkedTreapInternalFindNext(llt, *root);
      }
   }
   return(result);
}


#ifdef __cplusplus
}
#endif
