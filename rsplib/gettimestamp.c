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
#include "timeutilities.h"

#include <time.h>


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   char               str[256];
   bool               textOutput = false;
   unsigned long long offset     = 0;
   unsigned long long timeStamp;
   int                i;

   for(i = 1;i < argc;i++) {
      if(!(strcmp(argv[i], "-text"))) {
         textOutput = true;
      }
      else if(!(strncmp(argv[i], "-offset=", 8))) {
         offset = 1000ULL * (unsigned long long)atol((const char*)&argv[i][8]);
      }
      else {
         fprintf(stderr, "Usage: %s {-offset=Offset in ms} {-text}\n", argv[0]);
         exit(1);
      }
   }

   timeStamp = getMicroTime() + offset;
   if(!textOutput) {
      printf("%Lu\n", timeStamp);
   }
   else {
      const time_t     timeValue = timeStamp / 1000000;
      const struct tm* timePtr   = localtime(&timeValue);
      strftime((char*)&str, sizeof(str), "%d-%b-%Y %H:%M:%S", timePtr);
      printf(str);
      printf(".%06Lu, offset is %Luus\n", timeStamp % 1000000ULL, offset);
   }

   return(0);
}
