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
 * Copyright (C) 2002-2020 by Thomas Dreibholz
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
#include "asapinterthreadmessage.h"
#include "timeutilities.h"


/* ###### Constructor #################################################### */
struct ASAPInterThreadMessage* asapInterThreadMessageNew(
                                  struct RSerPoolMessage* request,
                                  const bool              responseExpected)
{
   struct ASAPInterThreadMessage* aitm;

   aitm = (struct ASAPInterThreadMessage*)malloc(sizeof(struct ASAPInterThreadMessage));
   if(aitm == NULL) {
      return(NULL);
   }

   aitm->Request                        = request;
   aitm->Response                       = NULL;
   aitm->ResponseExpected               = responseExpected;   /* Response from registrar? */
   aitm->Error                          = RSPERR_OKAY;
   aitm->TransmissionTrials             = 0;
   aitm->ResponseTimeoutTimeStamp       = 0;
   aitm->ResponseTimeoutNeedsScheduling = false;
   aitm->CreationTimeStamp              = getMicroTime();
   aitm->TransmissionTimeStamp          = 0;
   return(aitm);
}


/* ###### Destructor ##################################################### */
void asapInterThreadMessageDelete(struct ASAPInterThreadMessage* aitm)
{
   if(aitm) {
      rserpoolMessageDelete(aitm->Request);
      aitm->Request = NULL;
      if(aitm->Response) {
         rserpoolMessageDelete(aitm->Response);
         aitm->Response = NULL;
      }
      free(aitm);
   }
}
