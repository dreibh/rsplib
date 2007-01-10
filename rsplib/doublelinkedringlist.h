/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2007 by Thomas Dreibholz
 *
 * $Id$
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

#ifndef DOUBLELINKEDRINGLIST_H
#define DOUBLELINKEDRINGLIST_H

#include <stdlib.h>
#include "debug.h"


#ifdef __cplusplus
extern "C" {
#endif


struct DoubleLinkedRingListNode
{
   struct DoubleLinkedRingListNode* Prev;
   struct DoubleLinkedRingListNode* Next;
};


void doubleLinkedRingListNodeNew(struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListNodeDelete(struct DoubleLinkedRingListNode* node);


struct DoubleLinkedRingList
{
   struct DoubleLinkedRingListNode  Node;
   struct DoubleLinkedRingListNode* Head;
};


void doubleLinkedRingListNew(struct DoubleLinkedRingList* list);
void doubleLinkedRingListDelete(struct DoubleLinkedRingList* list);
void doubleLinkedRingListAddAfter(struct DoubleLinkedRingListNode* after,
                                  struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListAddHead(struct DoubleLinkedRingList*     list,
                                 struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListAddTail(struct DoubleLinkedRingList*     list,
                                 struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListRemNode(struct DoubleLinkedRingListNode* node);


#ifdef __cplusplus
}
#endif

#endif
