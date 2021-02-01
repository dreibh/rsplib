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
 * Copyright (C) 2002-2021 by Thomas Dreibholz
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
#include "notificationqueue.h"
#include "debug.h"


/* ###### Constructor #################################################### */
void notificationQueueNew(struct NotificationQueue* notificationQueue)
{
   notificationQueue->PreReadQueue  = NULL;
   notificationQueue->PreReadLast   = NULL;
   notificationQueue->PostReadQueue = NULL;
   notificationQueue->PostReadLast  = NULL;
   notificationQueue->EventMask     = 0;
}


/* ###### Destructor ##################################################### */
void notificationQueueDelete(struct NotificationQueue* notificationQueue)
{
   notificationQueueClear(notificationQueue);
}


/* ###### Clean up queue ################################################# */
void notificationQueueClear(struct NotificationQueue* notificationQueue)
{

   struct NotificationNode* next;

   while(notificationQueue->PreReadQueue) {
      next = notificationQueue->PreReadQueue->Next;
      free(notificationQueue->PreReadQueue);
      notificationQueue->PreReadQueue = next;
   }
   notificationQueue->PreReadLast = NULL;

   while(notificationQueue->PostReadQueue) {
      next = notificationQueue->PostReadQueue->Next;
      free(notificationQueue->PostReadQueue);
      notificationQueue->PostReadQueue = next;
   }
   notificationQueue->PostReadLast = NULL;
}


/* ###### Check, if there are notifications to read ###################### */
bool notificationQueueHasData(struct NotificationQueue* notificationQueue)
{
   return((notificationQueue->PreReadQueue != NULL) ||
          (notificationQueue->PostReadQueue != NULL));
}


/* ###### Enqueue notification ########################################### */
struct NotificationNode* notificationQueueEnqueueNotification(
                            struct NotificationQueue* notificationQueue,
                            const bool                isPreReadNotification,
                            const uint16_t            type)
{
   struct NotificationNode* notificationNode;

   /* ====== Only enqueue requested events =============================== */
   if((1 << type) & notificationQueue->EventMask) {
      notificationNode = (struct NotificationNode*)malloc(sizeof(struct NotificationNode));
      if(notificationNode) {
         /* ====== Set pending events appropriately ====================== */
         notificationNode->Content.rn_header.rn_type   = type;
         notificationNode->Content.rn_header.rn_flags  = 0x00;
         notificationNode->Content.rn_header.rn_length = sizeof(notificationNode->Content);

         /* ====== Add notification node ================================= */
         notificationNode->Next = NULL;
         if(isPreReadNotification) {
             if(notificationQueue->PreReadLast) {
                notificationQueue->PreReadLast->Next = notificationNode;
             }
             else {
                notificationQueue->PreReadQueue = notificationNode;
             }
             notificationQueue->PreReadLast = notificationNode;
         }
         else {
             if(notificationQueue->PostReadLast) {
                notificationQueue->PostReadLast->Next = notificationNode;
             }
             else {
                notificationQueue->PostReadQueue = notificationNode;
             }
             notificationQueue->PostReadLast = notificationNode;
         }
      }
   }
   else {
      notificationNode = NULL;
   }
   return(notificationNode);
}


/* ###### Dequeue notification ########################################### */
struct NotificationNode* notificationQueueDequeueNotification(
                            struct NotificationQueue* notificationQueue,
                            const bool                fromPreReadNotifications)
{
   struct NotificationNode* notificationNode;

   if(fromPreReadNotifications) {
      notificationNode = notificationQueue->PreReadQueue;
      if(notificationNode) {
         notificationQueue->PreReadQueue = notificationNode->Next;
      }
      if(notificationNode == notificationQueue->PreReadLast) {
         notificationQueue->PreReadLast = NULL;
      }
   }
   else {
      notificationNode = notificationQueue->PostReadQueue;
      if(notificationNode) {
         notificationQueue->PostReadQueue = notificationNode->Next;
      }
      if(notificationNode == notificationQueue->PostReadLast) {
         notificationQueue->PostReadLast = NULL;
      }
   }
   return(notificationNode);
}


/* ###### Destructor ##################################################### */
void notificationNodeDelete(struct NotificationNode* notificationNode)
{
   free(notificationNode);
}
