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
 * Copyright (C) 2002-2016 by Thomas Dreibholz
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

#include "rspregistrar.h"


#define TSHT_BUCKETS 64
#define TSHT_ENTRIES 16


/* ###### Check PU's permission for given operation using thresholds ##### */
bool registrarPoolUserHasPermissionFor(struct Registrar*               registrar,
                                       const int                       fd,
                                       const sctp_assoc_t              assocID,
                                       const unsigned int              action,
                                       const struct PoolHandle*        poolHandle,
                                       const PoolElementIdentifierType peIdentifier)
{
   static struct ST_CLASS(PoolUserNode)* nextPoolUserNode;
   struct ST_CLASS(PoolUserNode)*        poolUserNode;
   double                                rate;
   double                                threshold;
   unsigned long long                    now;

   if(nextPoolUserNode == NULL) {
      nextPoolUserNode = (struct ST_CLASS(PoolUserNode)*)malloc(sizeof(struct ST_CLASS(PoolUserNode)));
      if(nextPoolUserNode == NULL) {
         /* Giving permission here seems to be useful in case of trying to
            avoid DoS when - for some reason - the PR's memory is full. */
         return(true);
      }
   }

   ST_CLASS(poolUserNodeNew)(nextPoolUserNode, fd, assocID);
   poolUserNode = ST_CLASS(poolUserListAddOrUpdatePoolUserNode)(&registrar->PoolUsers, &nextPoolUserNode);
   CHECK(poolUserNode != NULL);

   now  = getMicroTime();
   rate = threshold = -1.0;
   switch(action) {
      case AHT_HANDLE_RESOLUTION:
         rate = ST_CLASS(poolUserNodeNoteHandleResolution)(poolUserNode, poolHandle, now, TSHT_BUCKETS, TSHT_ENTRIES);
         threshold = registrar->MaxHRRate;
       break;
      case AHT_ENDPOINT_UNREACHABLE:
         rate = ST_CLASS(poolUserNodeNoteEndpointUnreachable)(poolUserNode, poolHandle, peIdentifier, now, TSHT_BUCKETS, TSHT_ENTRIES);
         threshold = registrar->MaxEURate;
       break;
      default:
         CHECK(false);
   }

   /* printf("rate=%1.6f   threshold=%1.6f\n", rate, threshold); */
   return(rate <= threshold);
}
