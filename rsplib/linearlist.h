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

#ifndef LINEARLIST_H
#define LINEARLIST_H

#include <stdlib.h>
#include "doublelinkedringlist.h"
#include "debug.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned long long LinearListNodeValueType;


struct LinearListNode
{
   struct DoubleLinkedRingListNode Node;
   LinearListNodeValueType         Value;
};

struct LinearList
{
   struct DoubleLinkedRingList List;
   LinearListNodeValueType     ValueSum;
   size_t                      Elements;
   void                        (*PrintFunction)(const void* node, FILE* fd);
   int                         (*ComparisonFunction)(const void* node1, const void* node2);
};


/* ###### Initialize ##################################################### */
inline void linearListNodeNew(struct LinearListNode* node)
{
   doubleLinkedRingListNodeNew(&node->Node);
   node->Value = 0;
}


/* ###### Invalidate ##################################################### */
inline void linearListNodeDelete(struct LinearListNode* node)
{
   node->Value = 0;
   doubleLinkedRingListNodeDelete(&node->Node);
}


/* ###### Is node linked? ################################################ */
inline int linearListNodeIsLinked(struct LinearListNode* node)
{
   return(node->Node.Prev != NULL);
}


void linearListNew(struct LinearList* ll,
                   void               (*printFunction)(const void* node, FILE* fd),
                   int                (*comparisonFunction)(const void* node1, const void* node2));
void linearListDelete(struct LinearList* ll);
void linearListVerify(struct LinearList* ll);


/* ###### Is list empty? ################################################# */
inline int linearListIsEmpty(struct LinearList* ll)
{
   return((ll->List.Head == &ll->List.Node) &&
          (ll->List.Node.Prev == ll->List.Node.Next) &&
          (ll->List.Node.Prev == &ll->List.Node));
}


/* ###### Internal method for printing a node ############################ */
inline static void linearListPrintNode(struct LinearList*     ll,
                                       struct LinearListNode* node,
                                       FILE*                  fd)
{
   ll->PrintFunction(node, fd);
   fprintf(fd, " ");
#ifdef DEBUG
   fprintf(fd, " v=%Lu   ", node->Value);
#endif
}


/* ###### Print ########################################################## */
inline void linearListPrint(struct LinearList* ll, FILE* fd)
{
   struct LinearListNode* node;
   fprintf(fd, "List: ");
   doubleLinkedRingListTraverseForward(node, &ll->List) {
      linearListPrintNode(ll, node, fd);
   }
   fputs("\n", fd);
}


/* ###### Get first node ################################################# */
inline struct LinearListNode* linearListGetFirst(const struct LinearList* ll)
{
   if(ll->List.Node.Next != ll->List.Head) {
      return((struct LinearListNode*)ll->List.Node.Next);
   }
   return(NULL);
}


/* ###### Get last node ################################################## */
inline struct LinearListNode* linearListGetLast(const struct LinearList* ll)
{
   if(ll->List.Node.Prev != ll->List.Head) {
      return((struct LinearListNode*)ll->List.Node.Prev);
   }
   return(NULL);
}


/* ###### Get previous node ############################################## */
inline struct LinearListNode* linearListGetPrev(const struct LinearList* ll,
                                                struct LinearListNode*   node)
{
   if(node->Node.Prev != ll->List.Head) {
      return((struct LinearListNode*)node->Node.Prev);
   }
   return(NULL);
}


/* ###### Get next node ################################################## */
inline struct LinearListNode* linearListGetNext(const struct LinearList* ll,
                                                struct LinearListNode*   node)
{
   if(node->Node.Next != ll->List.Head) {
      return((struct LinearListNode*)node->Node.Next);
   }
   return(NULL);
}


/* ###### Find nearest previous node ##################################### */
inline struct LinearListNode* linearListGetNearestPrev(
                                 struct LinearList*     ll,
                                 struct LinearListNode* cmpNode)
{
   struct LinearListNode* node;
   doubleLinkedRingListTraverseBackward(node, &ll->List) {
      if(ll->ComparisonFunction(cmpNode, node) > 0) {
         return(node);
      }
   }
   return(NULL);
}


/* ###### Find nearest next node ######################################### */
inline struct LinearListNode* linearListGetNearestNext(
                                 struct LinearList*     ll,
                                 struct LinearListNode* cmpNode)
{
   struct LinearListNode* node;
   doubleLinkedRingListTraverseForward(node, &ll->List) {
      if(ll->ComparisonFunction(cmpNode, node) < 0) {
         return(node);
      }
   }
   return(NULL);
}


/* ###### Get number of elements ######################################### */
inline size_t linearListGetElements(const struct LinearList* ll)
{
   return(ll->Elements);
}


/* ###### Insert node #################################################### */
/*
   returns node, if node has been inserted. Otherwise, duplicate node
   already in treap is returned.
*/
inline struct LinearListNode* linearListInsert(struct LinearList*     ll,
                                               struct LinearListNode* newNode)
{
   struct LinearListNode* node;
   int                    cmp;
#ifdef DEBUG
   printf("insert: ");
   ll->PrintFunction(newNode);
   printf("\n");
#endif

   doubleLinkedRingListTraverseForward(node, &ll->List) {
      cmp = ll->ComparisonFunction(newNode, node);
      if(cmp < 0) {
         doubleLinkedRingListAddAfter(node->Node.Prev, &newNode->Node);
         ll->Elements++;
         ll->ValueSum += newNode->Value;
#ifdef DEBUG
         linearListPrint(ll, stdout);
#endif
#ifdef VERIFY
         linearListVerify(ll);
#endif
         return(newNode);
      }
      else if(cmp == 0) {
         return((struct LinearListNode*)node);
      }
   }

   doubleLinkedRingListAddTail(&ll->List, &newNode->Node);
   ll->Elements++;
   ll->ValueSum += newNode->Value;
#ifdef DEBUG
   linearListPrint(ll, stdout);
#endif
#ifdef VERIFY
   linearListVerify(ll);
#endif
   return(newNode);
}


/* ###### Find node ###################################################### */
inline struct LinearListNode* linearListFind(const struct LinearList*     ll,
                                             const struct LinearListNode* cmpNode)
{
   struct LinearListNode* node;
#ifdef DEBUG
   printf("find: ");
   ll->PrintFunction(cmpNode);
   printf("\n");
#endif

   doubleLinkedRingListTraverseForward(node, &ll->List) {
      if(ll->ComparisonFunction(cmpNode, node) == 0) {
         return(node);
      }
   }

   return(NULL);
}


/* ###### Remove node #################################################### */
inline struct LinearListNode* linearListRemove(struct LinearList*     ll,
                                               struct LinearListNode* node)
{
#ifdef DEBUG
   printf("remove: ");
   ll->PrintFunction(node);
   printf("\n");
#endif

   doubleLinkedRingListRemNode(&node->Node);
   ll->Elements--;
   ll->ValueSum -= node->Value;

#ifdef DEBUG
   linearListPrint(ll, stdout);
#endif
#ifdef DEBUG
   linearListVerify(ll);
#endif
   return(node);
}


/* ###### Get value sum ################################################## */
inline LinearListNodeValueType linearListGetValueSum(const struct LinearList* ll)
{
   return(ll->ValueSum);
}


/* ###### Select node by value ########################################### */
inline struct LinearListNode* linearListGetNodeByValue(struct LinearList*      ll,
                                                       LinearListNodeValueType value)
{
   struct LinearListNode* node;
   doubleLinkedRingListTraverseForward(node, &ll->List) {
      if(value < node->Value) {
         return(node);
      }
      value -= node->Value;
   }
   return(linearListGetLast(ll));
}


#ifdef __cplusplus
}
#endif

#endif
