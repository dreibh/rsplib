/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
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

#include "doublelinkedringlist.h"
#include "debug.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ###### Initialize ###################################################### */
void doubleLinkedRingListNodeNew(struct DoubleLinkedRingListNode* node)
{
   node->Prev = NULL;
   node->Next = NULL;
}

/* ###### Invalidate ###################################################### */
void doubleLinkedRingListNodeDelete(struct DoubleLinkedRingListNode* node)
{
   node->Prev = NULL;
   node->Next = NULL;
}


/* ###### Initialize ###################################################### */
void doubleLinkedRingListNew(struct DoubleLinkedRingList* list)
{
   list->Node.Prev     = &list->Node;
   list->Node.Next     = &list->Node;
   list->Head          = &list->Node;
}


/* ###### Invalidate ###################################################### */
void doubleLinkedRingListDelete(struct DoubleLinkedRingList* list)
{
   list->Head      = NULL;
   list->Node.Prev = NULL;
   list->Node.Next = NULL;
}


/* ###### Insert node after given node #################################### */
void doubleLinkedRingListAddAfter(struct DoubleLinkedRingListNode*        after,
                                         struct DoubleLinkedRingListNode* node)
{
   node->Next        = after->Next;
   node->Prev        = after;
   after->Next->Prev = node;
   after->Next       = node;
}


/* ###### Insert node at head ############################################# */
void doubleLinkedRingListAddHead(struct DoubleLinkedRingList*            list,
                                        struct DoubleLinkedRingListNode* node)
{
   doubleLinkedRingListAddAfter(&list->Node, node);
}


/* ###### Insert node at tail ############################################# */
void doubleLinkedRingListAddTail(struct DoubleLinkedRingList*            list,
                                        struct DoubleLinkedRingListNode* node)
{
   doubleLinkedRingListAddAfter(list->Node.Prev, node);
}


/* ###### Remove node ##################################################### */
void doubleLinkedRingListRemNode(struct DoubleLinkedRingListNode* node)
{
   node->Prev->Next = node->Next;
   node->Next->Prev = node->Prev;
   node->Prev = NULL;
   node->Next = NULL;
}


#ifdef __cplusplus
}
#endif
