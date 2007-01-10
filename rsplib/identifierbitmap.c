/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include "tdtypes.h"
#include "identifierbitmap.h"
#include "debug.h"


/* ###### Constructor #################################################### */
struct IdentifierBitmap* identifierBitmapNew(const size_t entries)
{
   const size_t slots = (entries + (IdentifierBitmapSlotsize - (entries % IdentifierBitmapSlotsize))) /
                           IdentifierBitmapSlotsize;
   struct IdentifierBitmap* identifierBitmap = (struct IdentifierBitmap*)malloc(sizeof(struct IdentifierBitmap) + (slots + 1) * sizeof(size_t));
   if(identifierBitmap) {
      memset(&identifierBitmap->Bitmap, 0, (slots + 1) * sizeof(size_t));
      identifierBitmap->Entries   = entries;
      identifierBitmap->Available = entries;
      identifierBitmap->Slots     = slots;
   }
   return(identifierBitmap);
}


/* ###### Destructor ##################################################### */
void identifierBitmapDelete(struct IdentifierBitmap* identifierBitmap)
{
   identifierBitmap->Entries = 0;
   free(identifierBitmap);
}


/* ###### Allocate ID #################################################### */
int identifierBitmapAllocateID(struct IdentifierBitmap* identifierBitmap)
{
   size_t i, j;
   int    id = -1;

   if(identifierBitmap->Available > 0) {
      i = 0;
      while(identifierBitmap->Bitmap[i] == ~((size_t)0)) {
         i++;
      }
      id = i * IdentifierBitmapSlotsize;

      j = 0;
      while((j < IdentifierBitmapSlotsize) &&
            (id < (int)identifierBitmap->Entries) &&
            (identifierBitmap->Bitmap[i] & (1 << j))) {
         j++;
         id++;
      }
      CHECK(id < (int)identifierBitmap->Entries);

      identifierBitmap->Bitmap[i] |= (1 << j);
      identifierBitmap->Available--;
   }

   return(id);
}


/* ###### Allocate specific ID ########################################### */
int identifierBitmapAllocateSpecificID(struct IdentifierBitmap* identifierBitmap,
                                       const int                id)
{
   size_t i, j;

   CHECK((id >= 0) && (id < (int)identifierBitmap->Entries));
   i = id / IdentifierBitmapSlotsize;
   j = id % IdentifierBitmapSlotsize;
   if(identifierBitmap->Bitmap[i] & (1 << j)) {
      return(-1);
   }
   identifierBitmap->Bitmap[i] |= (1 << j);
   identifierBitmap->Available--;
   return(id);
}


/* ###### Free ID ######################################################## */
void identifierBitmapFreeID(struct IdentifierBitmap* identifierBitmap, const int id)
{
   size_t i, j;

   CHECK((id >= 0) && (id < (int)identifierBitmap->Entries));
   i = id / IdentifierBitmapSlotsize;
   j = id % IdentifierBitmapSlotsize;
   CHECK(identifierBitmap->Bitmap[i] & (1 << j));
   identifierBitmap->Bitmap[i] &= ~(1 << j);
   identifierBitmap->Available++;
}
