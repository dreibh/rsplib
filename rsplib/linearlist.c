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

#include "linearlist.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ====== Initialize ====================================================== */
void linearListNew(struct LinearList* ll,
                   void               (*printFunction)(const void* node, FILE* fd),
                   int                (*comparisonFunction)(const void* node1, const void* node2))
{
   doubleLinkedRingListNew(&ll->List);
   ll->PrintFunction      = printFunction;
   ll->ComparisonFunction = comparisonFunction;
   ll->Elements           = 0;
   ll->ValueSum           = 0;
}


/* ====== Invalidate ====================================================== */
void linearListDelete(struct LinearList* ll)
{
   ll->ValueSum = 0;
   ll->Elements = 0;
   doubleLinkedRingListDelete(&ll->List);
}


/* ====== Verify structures =============================================== */
void linearListVerify(struct LinearList* ll)
{
   size_t                  counter  = 0;
   LinearListNodeValueType valueSum = 0;
   struct LinearListNode*  prevNode = NULL;
   struct LinearListNode*  node;

   doubleLinkedRingListTraverseForward(node, &ll->List) {
      counter++;
      valueSum += node->Value;

      if(counter > 1) {
         CHECK(prevNode != NULL);
         CHECK(prevNode->Node.Next == &node->Node);
         CHECK(node->Node.Prev == &prevNode->Node);
         CHECK(ll->ComparisonFunction(prevNode, node) < 0);
      }
      prevNode = node;
   }

   CHECK(counter == ll->Elements);
   CHECK(valueSum == ll->ValueSum);
}


#ifdef __cplusplus
}
#endif
