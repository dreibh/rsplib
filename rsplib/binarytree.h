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


/* ###### Initialize ##################################################### */
inline void binaryTreeNodeNew(struct BinaryTreeNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Invalidate ##################################################### */
inline void binaryTreeNodeDelete(struct BinaryTreeNode* node)
{
   node->Parent       = NULL;
   node->LeftSubtree  = NULL;
   node->RightSubtree = NULL;
   node->Value        = 0;
   node->ValueSum     = 0;
}


/* ###### Is node linked? ################################################ */
inline int binaryTreeNodeIsLinked(struct BinaryTreeNode* node)
{
   return(node->LeftSubtree != NULL);
}


void binaryTreeNew(struct BinaryTree* bt,
                        void                    (*printFunction)(const void* node, FILE* fd),
                        int                     (*comparisonFunction)(const void* node1, const void* node2));
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


/* ###### Is treap empty? ################################################ */
inline int binaryTreeIsEmpty(struct BinaryTree* bt)
{
   return(bt->TreeRoot == &bt->NullNode);
}


/* ###### Internal method for printing a node ############################# */
inline static void binaryTreePrintNode(struct BinaryTree*     bt,
                                       struct BinaryTreeNode* node,
                                       FILE*                  fd)
{
   bt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p v=%Lu vsum=%Lu", node, node->Value, node->ValueSum);
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
inline void binaryTreePrint(struct BinaryTree* bt, FILE* fd)
{
#ifdef DEBUG
   fprintf(fd, "root=%p null=%p   ", bt->TreeRoot, &bt->NullNode);
#endif
   binaryTreeInternalPrint(bt, bt->TreeRoot, fd);
   fputs("\n",fd );
}


/* ###### Get first node ################################################## */
inline struct BinaryTreeNode* binaryTreeGetFirst(const struct BinaryTree* bt)
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
inline struct BinaryTreeNode* binaryTreeGetLast(const struct BinaryTree* bt)
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
inline struct BinaryTreeNode* binaryTreeGetNearestPrev(
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
inline struct BinaryTreeNode* binaryTreeGetNearestNext(
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
inline size_t binaryTreeGetElements(const struct BinaryTree* bt)
{
   return(bt->Elements);
}


/* ###### Get prev node by walking through the tree ###################### */
inline struct BinaryTreeNode* binaryTreeGetPrev(const struct BinaryTree* bt,
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
inline struct BinaryTreeNode* binaryTreeGetNext(const struct BinaryTree* bt,
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
inline struct BinaryTreeNode* binaryTreeInsert(struct BinaryTree*     bt,
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
      // Important: The NullNode's parent pointer may be modified during rotations.
      // We reset it here. This is much more efficient than if-clauses in the
      // rotation functions.
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
inline struct BinaryTreeNode* binaryTreeRemove(struct BinaryTree*     bt,
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
inline struct BinaryTreeNode* binaryTreeFind(const struct BinaryTree*     bt,
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
inline BinaryTreeNodeValueType binaryTreeGetValueSum(
                                  const struct BinaryTree* bt)
{
   return(bt->TreeRoot->ValueSum);
}


struct BinaryTreeNode* binaryTreeGetNodeByValue(
                          struct BinaryTree*      bt,
                          BinaryTreeNodeValueType value);


#ifdef __cplusplus
}
#endif

#endif
