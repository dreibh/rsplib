/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2011 by Thomas Dreibholz
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

#include "poolhandlespacemanagement-basics.h"
#include "stringutilities.h"
#include "randomizer.h"


const PoolElementSeqNumberType SeqNumberStart = (~0) ^ 0xf;


/* ###### Get textual description of error code ########################## */
const char* poolHandlespaceManagementGetErrorDescription(const unsigned int errorCode)
{
   return(rserpoolErrorGetDescription(errorCode));
}


/* ###### Get pool element identifier #################################### */
PoolElementIdentifierType getPoolElementIdentifier()
{
   PoolElementIdentifierType poolElementIdentifier = 1 + (random32() % 0xfffffffe);
   return(poolElementIdentifier);
}


/* ###### Compute a hash from PH and PE ID ############################### */
unsigned int computePHPEHash(const struct PoolHandle*        poolHandle,
                             const PoolElementIdentifierType identifier)
{
   uint32_t hash = 0;
   uint32_t rest;

   const uint32_t* ph = (const uint32_t*)&poolHandle->Handle;
   ssize_t i = poolHandle->Size;
   while(i >= (ssize_t)sizeof(uint32_t)) {
      hash = hash ^ *ph;
      ph++;
      i -= sizeof(uint32_t);
   }
   if(i > 0) {
      rest = 0;
      memcpy(&rest, ph, i);
      hash = hash ^ rest;
   }

   hash = hash ^ (uint32_t)identifier;
   return(hash);
}
