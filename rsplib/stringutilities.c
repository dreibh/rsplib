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

#include "string.h"


/* ###### Length-checking strcpy() ###################################### */
int safestrcpy(char* dest, const char* src, const size_t size)
{
   if(size > 0) {
      strncpy(dest,src,size);
      dest[size - 1] = 0x00;
      return(strlen(dest) < size);
   }
   return(0);
}


/* ###### Length-checking strcat() ###################################### */
int safestrcat(char* dest, const char* src, const size_t size)
{
   const int l1  = strlen(dest);
   const int l2  = strlen(src);

   if(l1 + l2 < (int)size) {
      strcat(dest,src);
      return(1);
   }
   else if((int)size > l2) {
      strcat((char*)&dest[size - l2],src);
   }
   else {
      safestrcpy(dest,src,size);
   }
   return(0);
}
