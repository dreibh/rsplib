/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2010 by Thomas Dreibholz
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

#include "tdtypes.h"
#include "timeutilities.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>


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
      printf("%llu\n", timeStamp);
   }
   else {
      const time_t     timeValue = timeStamp / 1000000;
      const struct tm* timePtr   = localtime(&timeValue);
      strftime((char*)&str, sizeof(str), "%d-%b-%Y %H:%M:%S", timePtr);
      fputs(str, stdout);
      printf(".%06Lu, offset is %lluus\n", timeStamp % 1000000ULL, offset);
   }

   return(0);
}
