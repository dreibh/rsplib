/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2019 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#ifndef IDENTIFIERBITMAP_H
#define IDENTIFIERBITMAP_H

#include "tdtypes.h"


#ifdef __cplusplus
extern "C" {
#endif


struct IdentifierBitmap
{
   size_t        Entries;
   size_t        Available;
   size_t        Slots;
   unsigned long Bitmap[0];
};

#define IdentifierBitmapSlotsize (sizeof(unsigned long) * 8)


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
