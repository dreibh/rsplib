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

#include "leaflinkedredblacktree.h"
#include "debug.h"


#ifdef __cplusplus
{
#endif


/* ###### Initialize ##################################################### */
void leafLinkedRedBlackTreeNodeNew(struct LeafLinkedRedBlackTreeNode* node)
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
void leafLinkedRedBlackTreeNodeDelete(struct LeafLinkedRedBlackTreeNode* node)
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
int leafLinkedRedBlackTreeNodeIsLinked(struct LeafLinkedRedBlackTreeNode* node)
{
   return(node->LeftSubtree != NULL);
}


/* ##### Initialize ###################################################### */
void leafLinkedRedBlackTreeNew(struct LeafLinkedRedBlackTree* llrbt,
                               void                           (*printFunction)(const void* node, FILE* fd),
                               int                            (*comparisonFunction)(const void* node1, const void* node2))
{
   doubleLinkedRingListNew(&llrbt->List);
   llrbt->PrintFunction         = printFunction;
   llrbt->ComparisonFunction    = comparisonFunction;
   llrbt->NullNode.Parent       = &llrbt->NullNode;
   llrbt->NullNode.LeftSubtree  = &llrbt->NullNode;
   llrbt->NullNode.RightSubtree = &llrbt->NullNode;
   llrbt->NullNode.Color        = Black;
   llrbt->NullNode.Value        = 0;
   llrbt->NullNode.ValueSum     = 0;
   llrbt->Elements              = 0;
}


/* ##### Invalidate ###################################################### */
void leafLinkedRedBlackTreeDelete(struct LeafLinkedRedBlackTree* llrbt)
{
   llrbt->Elements = 0;
   llrbt->NullNode.Parent       = NULL;
   llrbt->NullNode.LeftSubtree  = NULL;
   llrbt->NullNode.RightSubtree = NULL;
   doubleLinkedRingListDelete(&llrbt->List);
}


/* ##### Update value sum ################################################ */
static void leafLinkedRedBlackTreeUpdateValueSum(struct LeafLinkedRedBlackTreeNode* node)
{
   node->ValueSum = node->LeftSubtree->ValueSum + node->Value + node->RightSubtree->ValueSum;
}


/* ##### Update value sum for node and all parents up to tree root ####### */
static void leafLinkedRedBlackTreeUpdateValueSumsUpToRoot(
                      struct LeafLinkedRedBlackTree*     llrbt,
                      struct LeafLinkedRedBlackTreeNode* node)
{
   while(node != &llrbt->NullNode) {
       leafLinkedRedBlackTreeUpdateValueSum(node);
       node = node->Parent;
   }
}


/* ###### Internal method for printing a node ############################# */
static void leafLinkedRedBlackTreePrintNode(struct LeafLinkedRedBlackTree*     llrbt,
                                            struct LeafLinkedRedBlackTreeNode* node,
                                            FILE*                              fd)
{
   llrbt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p c=%s v=%llu vsum=%llu",
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


/* ##### Internal printing function ###################################### */
void leafLinkedRedBlackTreeInternalPrint(struct LeafLinkedRedBlackTree*     llrbt,
                                         struct LeafLinkedRedBlackTreeNode* node,
                                         FILE*                              fd)
{
   if(node != &llrbt->NullNode) {
      leafLinkedRedBlackTreeInternalPrint(llrbt, node->LeftSubtree, fd);
      leafLinkedRedBlackTreePrintNode(llrbt, node, fd);
      leafLinkedRedBlackTreeInternalPrint(llrbt, node->RightSubtree, fd);
   }
}


/* ###### Is treap empty? ################################################ */
int leafLinkedRedBlackTreeIsEmpty(struct LeafLinkedRedBlackTree* llrbt)
{
   return(llrbt->NullNode.LeftSubtree == &llrbt->NullNode);
}


/* ###### Print treap ##################################################### */
void leafLinkedRedBlackTreePrint(struct LeafLinkedRedBlackTree* llrbt,
                                 FILE*                          fd)
{
#ifdef DEBUG
   fprintf(fd, "\n\nroot=%p[", llrbt->NullNode.LeftSubtree);
   llrbt->PrintFunction(llrbt->NullNode.LeftSubtree, fd);
   fprintf(fd, "] null=%p   \n", &llrbt->NullNode);
#endif
   leafLinkedRedBlackTreeInternalPrint(llrbt, llrbt->NullNode.LeftSubtree, fd);
   fputs("\n", fd);
}


/* ###### Get first node ################################################## */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetFirst(
                                      const struct LeafLinkedRedBlackTree* llrbt)
{
   struct DoubleLinkedRingListNode* node = llrbt->List.Node.Next;
   if(node != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)node);
   }
   return(NULL);
}


/* ###### Get last node ################################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetLast(
                                      const struct LeafLinkedRedBlackTree* llrbt)
{
   struct DoubleLinkedRingListNode* node = llrbt->List.Node.Prev;
   if(node != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)node);
   }
   return(NULL);
}


/* ###### Get previous node ############################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetPrev(
                                      const struct LeafLinkedRedBlackTree* llrbt,
                                      struct LeafLinkedRedBlackTreeNode*   node)
{
   struct DoubleLinkedRingListNode* prev = node->ListNode.Prev;
   if(prev != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)prev);
   }
   return(NULL);
}


/* ###### Get next node ################################################## */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNext(
                                      const struct LeafLinkedRedBlackTree* llrbt,
                                      struct LeafLinkedRedBlackTreeNode*   node)
{
   struct DoubleLinkedRingListNode* next = node->ListNode.Next;
   if(next != llrbt->List.Head) {
      return((struct LeafLinkedRedBlackTreeNode*)next);
   }
   return(NULL);
}


/* ###### Find nearest previous node ##################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNearestPrev(
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
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNearestNext(
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
size_t leafLinkedRedBlackTreeGetElements(const struct LeafLinkedRedBlackTree* llrbt)
{
   return(llrbt->Elements);
}


/* ###### Get prev node by walking through the tree (does *not* use list!) */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalFindPrev(
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
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalFindNext(
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


/* ###### Find node ####################################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeFind(
                                      const struct LeafLinkedRedBlackTree*     llrbt,
                                      const struct LeafLinkedRedBlackTreeNode* cmpNode)
{
#ifdef DEBUG
   printf("find: ");
   llrbt->PrintFunction(cmpNode, stdout);
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
LeafLinkedRedBlackTreeNodeValueType leafLinkedRedBlackTreeGetValueSum(
                                       const struct LeafLinkedRedBlackTree* llrbt)
{
   return(llrbt->NullNode.LeftSubtree->ValueSum);
}


/* ##### Rotation with left subtree ###################################### */
static void leafLinkedRedBlackTreeRotateLeft(
                      struct LeafLinkedRedBlackTreeNode* node)
{
   struct LeafLinkedRedBlackTreeNode* lower;
   struct LeafLinkedRedBlackTreeNode* lowleft;
   struct LeafLinkedRedBlackTreeNode* upparent;

   lower = node->RightSubtree;
   node->RightSubtree = lowleft = lower->LeftSubtree;
   lowleft->Parent = node;
   lower->Parent = upparent = node->Parent;

   if(node == upparent->LeftSubtree) {
      upparent->LeftSubtree = lower;
   } else {
      CHECK (node == upparent->RightSubtree);
      upparent->RightSubtree = lower;
   }

   lower->LeftSubtree = node;
   node->Parent = lower;

   leafLinkedRedBlackTreeUpdateValueSum(node);
   leafLinkedRedBlackTreeUpdateValueSum(node->Parent);
}


/* ##### Rotation with ripht subtree ##################################### */
static void leafLinkedRedBlackTreeRotateRight(
                      struct LeafLinkedRedBlackTreeNode* node)
{
   struct LeafLinkedRedBlackTreeNode* lower;
   struct LeafLinkedRedBlackTreeNode* lowright;
   struct LeafLinkedRedBlackTreeNode* upparent;

   lower = node->LeftSubtree;
   node->LeftSubtree = lowright = lower->RightSubtree;
   lowright->Parent = node;
   lower->Parent = upparent = node->Parent;

   if(node == upparent->RightSubtree) {
      upparent->RightSubtree = lower;
   } else {
      CHECK(node == upparent->LeftSubtree);
      upparent->LeftSubtree = lower;
   }

   lower->RightSubtree = node;
   node->Parent = lower;

   leafLinkedRedBlackTreeUpdateValueSum(node);
   leafLinkedRedBlackTreeUpdateValueSum(node->Parent);
}


/* ###### Insert ######################################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInsert(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* node)
{
   int                                cmpResult = -1;
   struct LeafLinkedRedBlackTreeNode* where     = llrbt->NullNode.LeftSubtree;
   struct LeafLinkedRedBlackTreeNode* parent    = &llrbt->NullNode;
   struct LeafLinkedRedBlackTreeNode* result;
   struct LeafLinkedRedBlackTreeNode* uncle;
   struct LeafLinkedRedBlackTreeNode* grandpa;
   struct LeafLinkedRedBlackTreeNode* prev;
#ifdef DEBUG
   printf("insert: ");
   llrbt->PrintFunction(node);
   printf("\n");
#endif


   /* ====== Find location of new node =================================== */
   while (where != &llrbt->NullNode) {
      parent = where;
      cmpResult = llrbt->ComparisonFunction(node, where);
      if(cmpResult < 0) {
         where = where->LeftSubtree;
      }
      else if(cmpResult > 0) {
         where = where->RightSubtree;
      }
      else {
         /* Node with same key is already available -> return. */
         result = where;
         goto finished;
      }
   }
   CHECK(where == &llrbt->NullNode);

   if(cmpResult < 0) {
      parent->LeftSubtree = node;
   }
   else {
      parent->RightSubtree = node;
   }


   /* ====== Link node =================================================== */
   node->Parent       = parent;
   node->LeftSubtree  = &llrbt->NullNode;
   node->RightSubtree = &llrbt->NullNode;
   node->ValueSum     = node->Value;
   prev = leafLinkedRedBlackTreeInternalFindPrev(llrbt, node);
   if(prev != &llrbt->NullNode) {
      doubleLinkedRingListAddAfter(&prev->ListNode, &node->ListNode);
   }
   else {
      doubleLinkedRingListAddHead(&llrbt->List, &node->ListNode);
   }
   llrbt->Elements++;
   result = node;


   /* ====== Update parent's value sum =================================== */
   leafLinkedRedBlackTreeUpdateValueSumsUpToRoot(llrbt, node->Parent);


   /* ====== Ensure red-black tree properties ============================ */
   node->Color = Red;
   while (parent->Color == Red) {
      grandpa = parent->Parent;
      if(parent == grandpa->LeftSubtree) {
         uncle = grandpa->RightSubtree;
         if(uncle->Color == Red) {
            parent->Color  = Black;
            uncle->Color   = Black;
            grandpa->Color = Red;
            node           = grandpa;
            parent         = grandpa->Parent;
         } else {
            if(node == parent->RightSubtree) {
               leafLinkedRedBlackTreeRotateLeft(parent);
               parent = node;
               CHECK(grandpa == parent->Parent);
            }
            parent->Color  = Black;
            grandpa->Color = Red;
            leafLinkedRedBlackTreeRotateRight(grandpa);
            break;
         }
      } else {
         uncle = grandpa->LeftSubtree;
         if(uncle->Color == Red) {
            parent->Color  = Black;
            uncle->Color   = Black;
            grandpa->Color = Red;
            node           = grandpa;
            parent         = grandpa->Parent;
         } else {
            if(node == parent->LeftSubtree) {
               leafLinkedRedBlackTreeRotateRight(parent);
               parent = node;
               CHECK(grandpa == parent->Parent);
            }
            parent->Color  = Black;
            grandpa->Color = Red;
            leafLinkedRedBlackTreeRotateLeft(grandpa);
            break;
         }
      }
   }
   llrbt->NullNode.LeftSubtree->Color = Black;


finished:
#ifdef DEBUG
   leafLinkedRedBlackTreePrint(llrbt);
#endif
#ifdef VERIFY
   leafLinkedRedBlackTreeVerify(llrbt);
#endif
   return(result);
}


/* ###### Remove ######################################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeRemove(
                                      struct LeafLinkedRedBlackTree*     llrbt,
                                      struct LeafLinkedRedBlackTreeNode* node)
{
   struct LeafLinkedRedBlackTreeNode*       child;
   struct LeafLinkedRedBlackTreeNode*       delParent;
   struct LeafLinkedRedBlackTreeNode*       parent;
   struct LeafLinkedRedBlackTreeNode*       sister;
   struct LeafLinkedRedBlackTreeNode*       next;
   struct LeafLinkedRedBlackTreeNode*       nextParent;
   enum LeafLinkedRedBlackTreeNodeColorType nextColor;
#ifdef DEBUG
   printf("remove: ");
   llrbt->PrintFunction(node);
   printf("\n");
#endif

   CHECK(leafLinkedRedBlackTreeNodeIsLinked(node));

   /* ====== Unlink node ================================================= */
   if((node->LeftSubtree != &llrbt->NullNode) && (node->RightSubtree != &llrbt->NullNode)) {
      next       = leafLinkedRedBlackTreeGetNext(llrbt, node);
      nextParent = next->Parent;
      nextColor  = next->Color;

      CHECK(next != &llrbt->NullNode);
      CHECK(next->Parent != &llrbt->NullNode);
      CHECK(next->LeftSubtree == &llrbt->NullNode);

      child         = next->RightSubtree;
      child->Parent = nextParent;
      if(nextParent->LeftSubtree == next) {
         nextParent->LeftSubtree = child;
      } else {
         CHECK(nextParent->RightSubtree == next);
         nextParent->RightSubtree = child;
      }


      delParent                  = node->Parent;
      next->Parent               = delParent;
      next->LeftSubtree          = node->LeftSubtree;
      next->RightSubtree         = node->RightSubtree;
      next->LeftSubtree->Parent  = next;
      next->RightSubtree->Parent = next;
      next->Color                = node->Color;
      node->Color                = nextColor;

      if(delParent->LeftSubtree == node) {
         delParent->LeftSubtree = next;
      } else {
         CHECK(delParent->RightSubtree == node);
         delParent->RightSubtree = next;
      }

      /* ====== Update parent's value sum ================================ */
      leafLinkedRedBlackTreeUpdateValueSumsUpToRoot(llrbt, nextParent);
      leafLinkedRedBlackTreeUpdateValueSumsUpToRoot(llrbt, next);
   } else {
      CHECK(node != &llrbt->NullNode);
      CHECK((node->LeftSubtree == &llrbt->NullNode) || (node->RightSubtree == &llrbt->NullNode));

      child         = (node->LeftSubtree != &llrbt->NullNode) ? node->LeftSubtree : node->RightSubtree;
      child->Parent = delParent = node->Parent;

      if(node == delParent->LeftSubtree) {
         delParent->LeftSubtree = child;
      } else {
         CHECK(node == delParent->RightSubtree);
         delParent->RightSubtree = child;
      }

      /* ====== Update parent's value sum ================================ */
      leafLinkedRedBlackTreeUpdateValueSumsUpToRoot(llrbt, delParent);
   }


   /* ====== Unlink node from list and invalidate pointers =============== */
   node->Parent       = NULL;
   node->RightSubtree = NULL;
   node->LeftSubtree  = NULL;
   doubleLinkedRingListRemNode(&node->ListNode);
   node->ListNode.Prev = NULL;
   node->ListNode.Next = NULL;
   CHECK(llrbt->Elements > 0);
   llrbt->Elements--;


   /* ====== Ensure red-black properties ================================= */
   if(node->Color == Black) {
      llrbt->NullNode.LeftSubtree->Color = Red;

      while (child->Color == Black) {
         parent = child->Parent;
         if(child == parent->LeftSubtree) {
            sister = parent->RightSubtree;
            CHECK(sister != &llrbt->NullNode);
            if(sister->Color == Red) {
               sister->Color = Black;
               parent->Color = Red;
               leafLinkedRedBlackTreeRotateLeft(parent);
               sister = parent->RightSubtree;
               CHECK(sister != &llrbt->NullNode);
            }
            if((sister->LeftSubtree->Color == Black) &&
               (sister->RightSubtree->Color == Black)) {
               sister->Color = Red;
               child = parent;
            } else {
               if(sister->RightSubtree->Color == Black) {
                  CHECK(sister->LeftSubtree->Color == Red);
                  sister->LeftSubtree->Color = Black;
                  sister->Color = Red;
                  leafLinkedRedBlackTreeRotateRight(sister);
                  sister = parent->RightSubtree;
                  CHECK(sister != &llrbt->NullNode);
               }
               sister->Color = parent->Color;
               sister->RightSubtree->Color = Black;
               parent->Color = Black;
               leafLinkedRedBlackTreeRotateLeft(parent);
               break;
            }
         } else {
            CHECK(child == parent->RightSubtree);
            sister = parent->LeftSubtree;
            CHECK(sister != &llrbt->NullNode);
            if(sister->Color == Red) {
               sister->Color = Black;
               parent->Color = Red;
               leafLinkedRedBlackTreeRotateRight(parent);
               sister = parent->LeftSubtree;
               CHECK(sister != &llrbt->NullNode);
            }
            if((sister->RightSubtree->Color == Black) &&
               (sister->LeftSubtree->Color == Black)) {
               sister->Color = Red;
               child = parent;
            } else {
               if(sister->LeftSubtree->Color == Black) {
                  CHECK(sister->RightSubtree->Color == Red);
                  sister->RightSubtree->Color = Black;
                  sister->Color = Red;
                  leafLinkedRedBlackTreeRotateLeft(sister);
                  sister = parent->LeftSubtree;
                  CHECK(sister != &llrbt->NullNode);
               }
               sister->Color = parent->Color;
               sister->LeftSubtree->Color = Black;
               parent->Color = Black;
               leafLinkedRedBlackTreeRotateRight(parent);
               break;
            }
         }
      }
      child->Color = Black;
      llrbt->NullNode.LeftSubtree->Color = Black;
   }


#ifdef DEBUG
    leafLinkedRedBlackTreePrint(llrbt);
#endif
#ifdef VERIFY
    leafLinkedRedBlackTreeVerify(llrbt);
#endif
   return(node);
}


/* ##### Get node by value ############################################### */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeGetNodeByValue(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      LeafLinkedRedBlackTreeNodeValueType value)
{
   struct LeafLinkedRedBlackTreeNode* node = llrbt->NullNode.LeftSubtree;
   for(;;) {
      if(value < node->LeftSubtree->ValueSum) {
         if(node->LeftSubtree != &llrbt->NullNode) {
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
         if(node->RightSubtree != &llrbt->NullNode) {
            value -= node->LeftSubtree->ValueSum + node->Value;
            node = node->RightSubtree;
         }
         else {
            break;
         }
      }
   }

   if(node !=  &llrbt->NullNode) {
      return(node);
   }
   return(NULL);
}


/* ##### Internal verification function ################################## */
static size_t leafLinkedRedBlackTreeInternalVerify(
                 struct LeafLinkedRedBlackTree*      llrbt,
                 struct LeafLinkedRedBlackTreeNode*  parent,
                 struct LeafLinkedRedBlackTreeNode*  node,
                 struct LeafLinkedRedBlackTreeNode** lastRedBlackTreeNode,
                 struct DoubleLinkedRingListNode**   lastListNode,
                 size_t*                             counter)
{
   struct LeafLinkedRedBlackTreeNode* prev;
   struct LeafLinkedRedBlackTreeNode* next;
   size_t                             leftHeight;
   size_t                             rightHeight;

   if(node != &llrbt->NullNode) {
      /* ====== Print node =============================================== */
#ifdef DEBUG
      printf("verifying ");
      leafLinkedRedBlackTreePrintNode(llrbt, node);
      puts("");
#endif

      /* ====== Correct parent? ========================================== */
      CHECK(node->Parent == parent);

      /* ====== Correct tree and heap properties? ======================== */
      if(node->LeftSubtree != &llrbt->NullNode) {
         CHECK(llrbt->ComparisonFunction(node, node->LeftSubtree) > 0);
      }
      if(node->RightSubtree != &llrbt->NullNode) {
         CHECK(llrbt->ComparisonFunction(node, node->RightSubtree) < 0);
      }

      /* ====== Is value sum okay? ======================================= */
      CHECK(node->ValueSum == node->LeftSubtree->ValueSum +
                              node->Value +
                              node->RightSubtree->ValueSum);

      /* ====== Is left subtree okay? ==================================== */
      leftHeight = leafLinkedRedBlackTreeInternalVerify(llrbt, node, node->LeftSubtree, lastRedBlackTreeNode, lastListNode, counter);

      /* ====== Is ring list okay? ======================================= */
      CHECK((*lastListNode)->Next != llrbt->List.Head);
      *lastListNode = (*lastListNode)->Next;
      CHECK(*lastListNode == &node->ListNode);

      /* ====== Is linking working correctly? ============================ */
      prev = leafLinkedRedBlackTreeInternalFindPrev(llrbt, node);
      if(prev != &llrbt->NullNode) {
         CHECK((*lastListNode)->Prev == &prev->ListNode);
      }
      else {
         CHECK((*lastListNode)->Prev == llrbt->List.Head);
      }

      next = leafLinkedRedBlackTreeInternalFindNext(llrbt, node);
      if(next != &llrbt->NullNode) {
         CHECK((*lastListNode)->Next == &next->ListNode);
      }
      else {
         CHECK((*lastListNode)->Next == llrbt->List.Head);
      }

      /* ====== Count elements =========================================== */
      (*counter)++;

      /* ====== Is right subtree okay? =================================== */
      rightHeight = leafLinkedRedBlackTreeInternalVerify(llrbt, node, node->RightSubtree, lastRedBlackTreeNode, lastListNode, counter);

      /* ====== Verify red-black property ================================ */
      CHECK((leftHeight != 0) || (rightHeight != 0));
      CHECK(leftHeight == rightHeight);
      if(node->Color == Red) {
         CHECK(node->LeftSubtree->Color == Black);
         CHECK(node->RightSubtree->Color == Black);
         return(leftHeight);
      }
      CHECK(node->Color == Black);
      return(leftHeight + 1);
   }
   return(1);
}


/* ##### Verify structures ############################################### */
void leafLinkedRedBlackTreeVerify(struct LeafLinkedRedBlackTree* llrbt)
{
   size_t                             counter              = 0;
   struct LeafLinkedRedBlackTreeNode* lastRedBlackTreeNode = NULL;
   struct DoubleLinkedRingListNode*   lastListNode         = &llrbt->List.Node;

   /* leafLinkedRedBlackTreePrint(llrbt); */

   CHECK(llrbt->NullNode.Color == Black);
   CHECK(llrbt->NullNode.Value == 0);
   CHECK(llrbt->NullNode.ValueSum == 0);

   CHECK(leafLinkedRedBlackTreeInternalVerify(llrbt, &llrbt->NullNode, llrbt->NullNode.LeftSubtree, &lastRedBlackTreeNode, &lastListNode, &counter) != 0);
   CHECK(counter == llrbt->Elements);
}


/* ##### Internal get nearest previous function ########################## */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalGetNearestPrev(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      struct LeafLinkedRedBlackTreeNode** root,
                                      struct LeafLinkedRedBlackTreeNode*  parent,
                                      struct LeafLinkedRedBlackTreeNode*  node)
{
   struct LeafLinkedRedBlackTreeNode* result;

   if(*root == &llrbt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &llrbt->NullNode;
      node->RightSubtree = &llrbt->NullNode;
      *root = node;
      result = leafLinkedRedBlackTreeInternalFindPrev(llrbt, node);
      *root = &llrbt->NullNode;
   }
   else {
      const int cmpResult = llrbt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedRedBlackTreeInternalGetNearestPrev(llrbt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = leafLinkedRedBlackTreeInternalGetNearestPrev(llrbt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = leafLinkedRedBlackTreeInternalFindPrev(llrbt, *root);
      }
   }
   return(result);
}


/* ##### Internal get nearest next function ############################## */
struct LeafLinkedRedBlackTreeNode* leafLinkedRedBlackTreeInternalGetNearestNext(
                                      struct LeafLinkedRedBlackTree*      llrbt,
                                      struct LeafLinkedRedBlackTreeNode** root,
                                      struct LeafLinkedRedBlackTreeNode*  parent,
                                      struct LeafLinkedRedBlackTreeNode*  node)
{
   struct LeafLinkedRedBlackTreeNode* result;

   if(*root == &llrbt->NullNode) {
      node->Parent       = parent;
      node->LeftSubtree  = &llrbt->NullNode;
      node->RightSubtree = &llrbt->NullNode;
      *root = node;
      result = leafLinkedRedBlackTreeInternalFindNext(llrbt, node);
      *root = &llrbt->NullNode;
   }
   else {
      const int cmpResult = llrbt->ComparisonFunction(node, *root);
      if(cmpResult < 0) {
         result = leafLinkedRedBlackTreeInternalGetNearestNext(llrbt, &((*root)->LeftSubtree), *root, node);
      }
      else if(cmpResult > 0) {
         result = leafLinkedRedBlackTreeInternalGetNearestNext(llrbt, &((*root)->RightSubtree), *root, node);
      }
      else {
         result = leafLinkedRedBlackTreeInternalFindNext(llrbt, *root);
      }
   }
   return(result);
}


#ifdef __cplusplus
}
#endif
