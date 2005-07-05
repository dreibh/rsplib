/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2005 by Thomas Dreibholz
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

#include "poolhandlespacechecksum.h"
#include "debug.h"


/* ###### Add checksums a and b ########################################## */
HandlespaceChecksumType handlespaceChecksumAdd(const HandlespaceChecksumType a,
                                               const HandlespaceChecksumType b)
{
   return(~(~a + ~b));
}


/* ###### Subtract checksum b from a ##################################### */
HandlespaceChecksumType handlespaceChecksumSub(const HandlespaceChecksumType a,
                                               const HandlespaceChecksumType b)
{
   return(~(~a - ~b));
}


/* ###### Compute handlespace checksum ################################### */
HandlespaceChecksumType handlespaceChecksumCompute(HandlespaceChecksumType checksum,
                                                   const char*             buffer,
                                                   size_t                  size)
{
   HandlespaceChecksumType* addr  = (HandlespaceChecksumType*)buffer;
   HandlespaceChecksumType  sum   = 0;
   HandlespaceChecksumType  tmp;

   while(size >= sizeof(*addr))  {
      sum += *addr++;
      size -= sizeof(*addr);
   }

   if(size > 0) {
      tmp = 0;
      memcpy(&tmp, addr, size);
      sum += tmp;
   }

   checksum = ~sum;
   return(checksum);
}
