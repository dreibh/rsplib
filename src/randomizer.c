/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2024 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#include "randomizer.h"
#include "timeutilities.h"

#include <math.h>
#include <stdlib.h>


#ifdef NDEBUG
#undef min
#undef max
#include <omnetpp.h>

using namespace omnetpp;

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
#warning Using OMNeT++ random generator instead of time-seeded one!
   const double value = uniform(getSimulation()->getContextModule()->getRNG(0), 0.0, (double)0xffffffff);
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


/* ###### Get double random value ######################################## */
double randomDouble()
{
   return( (double)random32() / (double)4294967296.0 );
}


/* ###### Get exponential-distributed double random value ################ */
double randomExpDouble(const double p)
{
   return( -p * log(randomDouble()) );
}
