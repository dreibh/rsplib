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
 * Copyright (C) 2002-2023 by Thomas Dreibholz
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

#ifndef ASAPINTERTHREADMESSAGE_H
#define ASAPINTERTHREADMESSAGE_H

#include "tdtypes.h"
#include "interthreadmessageport.h"
#include "rserpoolmessage.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ASAPInterThreadMessage
{
   struct InterThreadMessageNode Node;
   struct RSerPoolMessage*       Request;
   struct RSerPoolMessage*       Response;
   size_t                        TransmissionTrials;
   unsigned int                  Error;
  
   unsigned long long            CreationTimeStamp;
   unsigned long long            TransmissionTimeStamp;

   unsigned long long            ResponseTimeoutTimeStamp;
   bool                          ResponseExpected;
   bool                          ResponseTimeoutNeedsScheduling;
};


struct ASAPInterThreadMessage* asapInterThreadMessageNew(
                                  struct RSerPoolMessage* request,
                                  const bool              responseExpected);
void asapInterThreadMessageDelete(struct ASAPInterThreadMessage* aitm);


#ifdef __cplusplus
}
#endif

#endif
