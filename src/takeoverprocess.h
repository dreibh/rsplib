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
 * Copyright (C) 2003-2025 by Thomas Dreibholz
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

#ifndef TAKEOVERPROCESS_H
#define TAKEOVERPROCESS_H

#include "poolhandlespacemanagement.h"

#ifdef __cplusplus
extern "C" {
#endif


struct TakeoverProcess
{
   size_t                  OutstandingAcks;
   RegistrarIdentifierType PeerIDArray[0];
};


struct TakeoverProcess* takeoverProcessNew(
                           const RegistrarIdentifierType        targetID,
                           struct ST_CLASS(PeerListManagement)* peerList);
void takeoverProcessDelete(struct TakeoverProcess* takeoverProcess);

size_t takeoverProcessGetOutstandingAcks(const struct TakeoverProcess* takeoverProcess);
size_t takeoverProcessAcknowledge(struct TakeoverProcess*       takeoverProcess,
                                  const RegistrarIdentifierType targetID,
                                  const RegistrarIdentifierType acknowledgerID);


#ifdef __cplusplus
}
#endif

#endif
