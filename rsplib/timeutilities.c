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

#include "timeutilities.h"
#include <sys/time.h>
#include <time.h>


/* ###### Get current timer ############################################## */
unsigned long long getMicroTime()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return(((unsigned long long)tv.tv_sec * (unsigned long long)1000000) +
         (unsigned long long)tv.tv_usec);
}


/* ###### Print time stamp ############################################### */
void printTimeStamp(FILE* fd)
{
   char                     str[64];
   const unsigned long long microTime = getMicroTime();
   const time_t             timeStamp = microTime / 1000000;
   const struct tm*         timeptr   = localtime(&timeStamp);

   strftime((char*)&str,sizeof(str),"%d-%b-%Y %H:%M:%S",timeptr);
   fprintf(fd,str);
   fprintf(fd,".%04d: ",(unsigned int)(microTime % 1000000) / 100);
}
