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


static void binaryTreePrintNode(struct BinaryTree*     bt,
                                struct BinaryTreeNode* node,
                                FILE*                  fd);


/* ###### Initialize ##################################################### */
void binaryTreeNodeNew(struct BinaryTreeNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Invalidate ##################################################### */
void binaryTreeNodeDelete(struct BinaryTreeNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Is node linked? ################################################ */
int binaryTreeNodeIsLinked(struct BinaryTreeNode* node)
{
   return(node->LeftSubtree != NULL);
}


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
   bt->Elements              = 0;
   bt->TreeRoot              = NULL;
   bt->NullNode.Parent       = NULL;
   bt->NullNode.LeftSubtree  = NULL;
   bt->NullNode.RightSubtree = NULL;
}


/* ##### Update value sum ################################################ */
static void binaryTreeUpdateValueSum(struct BinaryTreeNode* node)
{
   node->ValueSum = node->LeftSubtree->ValueSum + node->Value + node->RightSubtree->ValueSum;
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


/* ###### Is treap empty? ################################################ */
int binaryTreeIsEmpty(struct BinaryTree* bt)
{
   return(bt->TreeRoot == &bt->NullNode);
}


/* ###### Internal method for printing a node ############################# */
static void binaryTreePrintNode(struct BinaryTree*     bt,
                                struct BinaryTreeNode* node,
                                FILE*                  fd)
{
   bt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p v=%llu vsum=%llu", node, node->Value, node->ValueSum);
   if(node->LeftSubtree != &bt->NullNode) {
      fprintf(fd, " l=%p", node->LeftSubtree);
   }
   else {
      fprintf(fd, " l=()");
   }
   if(node->RightSubtree != &bt->NullNode) {
      fprintf(fd, " r=%p", node->RightSubtree);
   }
   else {
      fprintf(fd, " r=()");
   }
   if(node->Parent != &bt->NullNode) {
      fprintf(fd, " p=%p ", node->Parent);
   }
   else {
      fprintf(fd, " p=())   ");
   }
#endif
}


/* ###### Print treap ##################################################### */
void binaryTreePrint(struct BinaryTree* bt, FILE* fd)
{
#ifdef DEBUG
   fprintf(fd, "root=%p null=%p   ", bt->TreeRoot, &bt->NullNode);
#endif
   binaryTreeInternalPrint(bt, bt->TreeRoot, fd);
   fputs("\n",fd );
}


/* ###### Get first node ################################################## */
struct BinaryTreeNode* binaryTreeGetFirst(const struct BinaryTree* bt)
{
   struct BinaryTreeNode* node = bt->TreeRoot;
   while(node->LeftSubtree != &bt->NullNode) {
      node = node->LeftSubtree;
   }
   if(node != &bt->NullNode) {
      return(node);
   }
   return(NULL);
}


/* ###### Get last node ################################################### */
struct BinaryTreeNode* binaryTreeGetLast(const struct BinaryTree* bt)
{
   struct BinaryTreeNode* node = bt->TreeRoot;
   while(node->RightSubtree != &bt->NullNode) {
      node = node->RightSubtree;
   }
   if(node != &bt->NullNode) {
      return(node);
   }
   return(NULL);
}


/* ###### Find nearest previous node ##################################### */
struct BinaryTreeNode* binaryTreeGetNearestPrev(
                                 struct BinaryTree*     bt,
                                 struct BinaryTreeNode* cmpNode)
{
   struct BinaryTreeNode* result;
   result = binaryTreeInternalGetNearestPrev(
               bt, &bt->TreeRoot, &bt->NullNode, cmpNode);
   if(result != &bt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Find nearest next node ######################################### */
struct BinaryTreeNode* binaryTreeGetNearestNext(
                                 struct BinaryTree*     bt,
                                 struct BinaryTreeNode* cmpNode)
{
   struct BinaryTreeNode* result;
   result = binaryTreeInternalGetNearestNext(
               bt, &bt->TreeRoot, &bt->NullNode, cmpNode);
   if(result != &bt->NullNode) {
      return(result);
   }
   return(NULL);
}


/* ###### Get number of elements ########################################## */
size_t binaryTreeGetElements(const struct BinaryTree* bt)
{
   return(bt->Elements);
}


/* ###### Get prev node by walking through the tree ###################### */
struct BinaryTreeNode* binaryTreeGetPrev(const struct BinaryTree* bt,
                                                struct BinaryTreeNode*   cmpNode)
{
   struct BinaryTreeNode* node = cmpNode->LeftSubtree;
   struct BinaryTreeNode* parent;

   if(node != &bt->NullNode) {
      while(node->RightSubtree != &bt->NullNode) {
         node = node->RightSubtree;
      }
      if(node != &bt->NullNode) {
         return(node);
      }
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &bt->NullNode) && (node == parent->LeftSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      if(parent != &bt->NullNode) {
         return(parent);
      }
   }
   return(NULL);
}


/* ###### Get next node by walking through the tree ###################### */
struct BinaryTreeNode* binaryTreeGetNext(const struct BinaryTree* bt,
                                                struct BinaryTreeNode*   cmpNode)
{
   struct BinaryTreeNode* node = cmpNode->RightSubtree;
   struct BinaryTreeNode* parent;

   if(node != &bt->NullNode) {
      while(node->LeftSubtree != &bt->NullNode) {
         node = node->LeftSubtree;
      }
      if(node != &bt->NullNode) {
         return(node);
      }
      return(node);
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &bt->NullNode) && (node == parent->RightSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      if(parent != &bt->NullNode) {
         return(parent);
      }
   }
   return(NULL);
}


/* ###### Insert node ##################################################### */
/*
   returns node, if node has been inserted. Otherwise, duplicate node
   already in treap is returned.
*/
struct BinaryTreeNode* binaryTreeInsert(struct BinaryTree*     bt,
                                               struct BinaryTreeNode* node)
{
   struct BinaryTreeNode* result;
#ifdef DEBUG
   printf("insert: ");
   bt->PrintFunction(node, stdout);
   printf("\n");
#endif

   result = binaryTreeInternalInsert(bt, &bt->TreeRoot, &bt->NullNode, node);
   if(result == node) {
      /*
         Important: The NullNode's parent pointer may be modified during rotations.
         We reset it here. This is much more efficient than if-clauses in the
         rotation functions.
      */
      bt->NullNode.Parent = &bt->NullNode;
#ifdef DEBUG
      binaryTreePrint(bt);
#endif
#ifdef VERIFY
      binaryTreeVerify(bt);
#endif
   }
   return(result);
}


/* ###### Remove node ##################################################### */
struct BinaryTreeNode* binaryTreeRemove(struct BinaryTree*     bt,
                                               struct BinaryTreeNode* node)
{
   struct BinaryTreeNode* result;
#ifdef DEBUG
   printf("remove: ");
   bt->PrintFunction(node);
   printf("\n");
#endif

   result = binaryTreeInternalRemove(bt, &bt->TreeRoot, node);

#ifdef DEBUG
   binaryTreePrint(bt);
#endif
#ifdef VERIFY
   binaryTreeVerify(bt);
#endif
   return(result);
}


/* ###### Find node ####################################################### */
struct BinaryTreeNode* binaryTreeFind(const struct BinaryTree*     bt,
                                             const struct BinaryTreeNode* cmpNode)
{
#ifdef DEBUG
   printf("find: ");
   bt->PrintFunction(cmpNode);
   printf("\n");
#endif

   struct BinaryTreeNode* node = bt->TreeRoot;
   while(node != &bt->NullNode) {
      const int cmpResult = bt->ComparisonFunction(cmpNode, node);
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
BinaryTreeNodeValueType binaryTreeGetValueSum(
                                  const struct BinaryTree* bt)
{
   return(bt->TreeRoot->ValueSum);
}


#ifdef __cplusplus
}
#endif
