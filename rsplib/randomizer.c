/*
 *  $Id: randomizer.c,v 1.10 2004/11/12 00:16:51 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Randomizer
 *
 */

#include "randomizer.h"
#include "timeutilities.h"

#include <stdio.h>
#include <stdlib.h>


#ifdef NDEBUG
#include <omnetpp.h>
#else
/*
   It is tried to use /dev/urandom as random source first, since
   it provides high-quality random numbers. If /dev/urandom is not
   available, use the clib's random() function with a seed given
   by the current microseconds time. However, the random number
   quality is much lower since the seed may be easily predictable.
*/



#define RS_TRY_DEVICE 0
#define RS_DEVICE     1
#define RS_CLIB       2



static int   RandomSource = RS_TRY_DEVICE;
static FILE* RandomDevice = NULL;
#endif



/* ###### Get 8-bit random value ######################################### */
uint8_t random8()
{
   return((uint8_t)random32());
}


/* ###### Get 16-bit random value ######################################## */
uint16_t random16()
{
   return((uint16_t)random32());
}


/* ###### Get 64-bit random value ######################################## */
uint64_t random64()
{
   return( (((uint64_t)random32()) << 32) | (uint64_t)random32() );
}


/* ###### Get 32-bit random value ######################################## */
uint32_t random32()
{
#ifdef NDEBUG
#warning Using OMNeT++ random generator
   const double value = uniform(0.0, (double)0xffffffff);
   return((uint32_t)rint(value));
#else
  uint32_t number;

   switch(RandomSource) {
      case RS_DEVICE:
         if(fread(&number, sizeof(number), 1, RandomDevice) == 1) {
            return(number);
         }
         RandomSource = RS_CLIB;
      case RS_CLIB:
         return(random());
       break;
      case RS_TRY_DEVICE:
         RandomDevice = fopen("/dev/urandom", "r");
         if(RandomDevice != NULL) {
            if(fread(&number, sizeof(number), 1, RandomDevice) == 1) {
               srandom(number);
               RandomSource = RS_DEVICE;
               return(number);
            }
            fclose(RandomDevice);
         }
         RandomSource = RS_CLIB;
         srandom((unsigned int)(getMicroTime() & (uint64_t)0xffffffff));
      break;
   }
   return(random());
#endif
}
