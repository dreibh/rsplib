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

#include "binarytree.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ##### Initialize ###################################################### */
void binaryTreeNew(struct BinaryTree* bt,
                   void               (*printFunction)(const void* node, FILE* fd),
                   int                (*comparisonFunction)(const void* node1, const void* node2))
{
   bt->PrintFunction         = printFunction;
   bt->ComparisonFunction    = comparisonFunction;
   bt->NullNode.Parent       = &bt->NullNode;
   bt->NullNode.LeftSubtree  = &bt->NullNode;
   bt->NullNode.RightSubtree = &bt->NullNode;
   bt->NullNode.Value        = 0;
   bt->NullNode.ValueSum     = 0;
   bt->TreeRoot              = &bt->NullNode;
   bt->Elements              = 0;
}


/* ##### Invalidate ###################################################### */
void binaryTreeDelete(struct BinaryTree* bt)
{
   bt->Elements = 0;
   bt->TreeRoot              = NULL;
   bt->NullNode.Parent       = NULL;
   bt->NullNode.LeftSubtree  = NULL;
   bt->NullNode.RightSubtree = NULL;
}


/* ##### Update value sum ################################################ */
inline static void binaryTreeUpdateValueSum(struct BinaryTreeNode* node)
{
   node->ValueSum = node->LeftSubtree->ValueSum + node->Value + node->RightSubtree->ValueSum;
}


/* ##### Update value sum of left and right subtree + current ones ####### */
inline static void binaryTreeUpdateLeftAndRightValueSums(
                      struct BinaryTreeNode* node)
{
   binaryTreeUpdateValueSum(node->LeftSubtree);
   binaryTreeUpdateValueSum(node->RightSubtree);
   binaryTreeUpdateValueSum(node);
}


/* ##### Internal printing function ###################################### */
void binaryTreeInternalPrint(struct BinaryTree*     bt,
                             struct BinaryTreeNode* node,
                             FILE*                  fd)
{
   if(node != &bt->NullNode) {
      binaryTreeInternalPrint(bt, node->LeftSubtree, fd);
      binaryTreePrintNode(bt, node, fd);
      binaryTreeInternalPrint(bt, node->RightSubtree, fd);
   }
}


/* ##### Internal insertion function ##################################### */
struct BinaryTreeNode* binaryTreeInternalInsert(
                          struct BinaryTree*      bt,
                          struct BinaryTreeNode** root,
                          struct BinaryTreeNode*  parent,
                          struct BinaryTreeNode*  node)
{
   struct BinaryTreeNode* result;

   if(*root == &bt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &bt->NullNode;
      node->RightSubtree = &bt->NullNode;
      *root = node;
      bt->Elements++;
      binaryTreeUpdateValueSum(*root);
      result = node;
   }
   else {
      const int cmpResult = bt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = binaryTreeInternalInsert(bt, &((*root)->LeftSubtree), *root, node);
         binaryTreeUpdateValueSum(*root);
      }
      else if(cmpResult > 0) {
         result = binaryTreeInternalInsert(bt, &((*root)->RightSubtree), *root, node);
         binaryTreeUpdateValueSum(*root);
      }
      else {
         /* Tried to insert duplicate -> return. */
         result = *root;
      }
   }
   return(result);
}


/* ##### Internal removal function ####################################### */
struct BinaryTreeNode* binaryTreeInternalRemove(
                          struct BinaryTree*      bt,
                          struct BinaryTreeNode** root,
                          struct BinaryTreeNode*  node)
{
   struct BinaryTreeNode* result = NULL;
   struct BinaryTreeNode* prev;

   if(*root != &bt->NullNode) {
      const int cmpResult = bt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = binaryTreeInternalRemove(bt, &((*root)->LeftSubtree), node);
         binaryTreeUpdateValueSum(*root);
      }
      else if(cmpResult > 0) {
         result = binaryTreeInternalRemove(bt, &((*root)->RightSubtree), node);
         binaryTreeUpdateValueSum(*root);
      }
      else {
         if(((*root)->LeftSubtree != &bt->NullNode) &&
            ((*root)->RightSubtree !=&bt->NullNode)) {
            prev = (*root)->LeftSubtree;
            while(prev->RightSubtree != &bt->NullNode) {
               prev = prev->RightSubtree;
            }
            (*root)->RightSubtree->Parent = prev;
            prev->RightSubtree = (*root)->RightSubtree;
            (*root)->LeftSubtree->Parent = (*root)->Parent;
            *root = (*root)->LeftSubtree;
         }
         else {
            if((*root)->LeftSubtree != &bt->NullNode) {
               (*root)->LeftSubtree->Parent = (*root)->Parent;
               *root = (*root)->LeftSubtree;
            }
            else if((*root)->RightSubtree != &bt->NullNode) {
               (*root)->RightSubtree->Parent = (*root)->Parent;
               *root = (*root)->RightSubtree;
            }
            else {
               *root = &bt->NullNode;
            }
         }

         node->Parent       = NULL;
         node->LeftSubtree  = NULL;
         node->RightSubtree = NULL;
         CHECK(bt->Elements >= 1);
         bt->Elements--;
         result = node;
      }
   }
   return(result);
}


/* ##### Get node by value ############################################### */
struct BinaryTreeNode* binaryTreeGetNodeByValue(
                          struct BinaryTree*      bt,
                          BinaryTreeNodeValueType value)
{
   struct BinaryTreeNode* node = bt->TreeRoot;
   for(;;) {
      if(value < node->LeftSubtree->ValueSum) {
         if(node->LeftSubtree != &bt->NullNode) {
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
         if(node->RightSubtree != &bt->NullNode) {
            value -= node->LeftSubtree->ValueSum + node->Value;
            node = node->RightSubtree;
         }
         else {
            break;
         }
      }
   }

   if(node !=  &bt->NullNode) {
      return(node);
   }
   return(NULL);
}


/* ##### Internal verification function ################################## */
static void binaryTreeInternalVerify(struct BinaryTree*      bt,
                                     struct BinaryTreeNode*  parent,
                                     struct BinaryTreeNode*  node,
                                     struct BinaryTreeNode** lastTreeNode,
                                     size_t*                 counter)
{
   if(node != &bt->NullNode) {
      /* ====== Print node =============================================== */
#ifdef DEBUG
      printf("verifying ");
      binaryTreePrintNode(bt, node);
      puts("");
#endif

      /* ====== Correct parent? ========================================== */
      CHECK(node->Parent == parent);

      /* ====== Correct tree and heap properties? ======================== */
      if(node->LeftSubtree != &bt->NullNode) {
         CHECK(bt->ComparisonFunction(node, node->LeftSubtree) > 0);
      }
      if(node->RightSubtree != &bt->NullNode) {
         CHECK(bt->ComparisonFunction(node, node->RightSubtree) < 0);
      }

      /* ====== Is value sum okay? ======================================= */
      CHECK(node->ValueSum == node->LeftSubtree->ValueSum +
                              node->Value +
                              node->RightSubtree->ValueSum);

      /* ====== Is left subtree okay? ==================================== */
      binaryTreeInternalVerify(bt, node, node->LeftSubtree, lastTreeNode, counter);

      /* ====== Count elements =========================================== */
      (*counter)++;

      /* ====== Is right subtree okay? =================================== */
      binaryTreeInternalVerify(bt, node, node->RightSubtree, lastTreeNode, counter);
   }
}


/* ##### Verify structures ############################################### */
void binaryTreeVerify(struct BinaryTree* bt)
{
   size_t                 counter      = 0;
   struct BinaryTreeNode* lastTreeNode = NULL;

   /* binaryTreePrint(bt); */

   CHECK(bt->NullNode.Parent       == &bt->NullNode);
   CHECK(bt->NullNode.LeftSubtree  == &bt->NullNode);
   CHECK(bt->NullNode.RightSubtree == &bt->NullNode);
   CHECK(bt->NullNode.Value        == 0);
   CHECK(bt->NullNode.ValueSum     == 0);

   binaryTreeInternalVerify(bt, &bt->NullNode, bt->TreeRoot, &lastTreeNode, &counter);
   CHECK(counter == bt->Elements);
}


/* ##### Internal get nearest previous function ########################## */
struct BinaryTreeNode* binaryTreeInternalGetNearestPrev(
                          struct BinaryTree*      bt,
                          struct BinaryTreeNode** root,
                          struct BinaryTreeNode*  parent,
                          struct BinaryTreeNode*  node)
{
   struct BinaryTreeNode* result;

   if(*root == &bt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &bt->NullNode;
      node->RightSubtree = &bt->NullNode;
      *root = node;
      result = binaryTreeGetPrev(bt, node);
      *root = &bt->NullNode;
   }
   else {
      const int cmpResult = bt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = binaryTreeInternalGetNearestPrev(bt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = binaryTreeInternalGetNearestPrev(bt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = binaryTreeGetPrev(bt, *root);
      }
   }
   return(result);
}


/* ##### Internal get nearest next function ############################## */
struct BinaryTreeNode* binaryTreeInternalGetNearestNext(
                          struct BinaryTree*      bt,
                          struct BinaryTreeNode** root,
                          struct BinaryTreeNode*  parent,
                          struct BinaryTreeNode*  node)
{
   struct BinaryTreeNode* result;

   if(*root == &bt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &bt->NullNode;
      node->RightSubtree = &bt->NullNode;
      *root = node;
      result = binaryTreeGetNext(bt, node);
      *root = &bt->NullNode;
   }
   else {
      const int cmpResult = bt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = binaryTreeInternalGetNearestNext(bt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = binaryTreeInternalGetNearestNext(bt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = binaryTreeGetNext(bt, *root);
      }
   }
   return(result);
}


#ifdef __cplusplus
}
#endif
