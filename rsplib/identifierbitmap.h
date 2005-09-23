/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#ifndef IDENTIFIERBITMAP_H
#define IDENTIFIERBITMAP_H

#include "tdtypes.h"


#ifdef __cplusplus
extern "C" {
#endif


struct IdentifierBitmap
{
   size_t Entries;
   size_t Available;

   size_t Slots;
   size_t Bitmap[0];
};

#define IdentifierBitmapSlotsize (sizeof(size_t) * 8)


struct IdentifierBitmap* identifierBitmapNew(const size_t entries);
void identifierBitmapDelete(struct IdentifierBitmap* identifierBitmap);
int identifierBitmapAllocateID(struct IdentifierBitmap* identifierBitmap);
int identifierBitmapAllocateSpecificID(struct IdentifierBitmap* identifierBitmap,
                                       const int                id);
void identifierBitmapFreeID(struct IdentifierBitmap* identifierBitmap, const int id);


#ifdef __cplusplus
}
#endif

#endif
