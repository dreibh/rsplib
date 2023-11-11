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
 * Copyright (C) 2002-2024 by Thomas Dreibholz
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

#ifndef NOTIFICATIONQUEUE_H
#define NOTIFICATIONQUEUE_H

#include "tdtypes.h"
#include "rserpool.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Notification event types */
#define NET_SESSION_CHANGE    (1 << RSERPOOL_SESSION_CHANGE)
#define NET_FAILOVER          (1 << RSERPOOL_FAILOVER)
#define NET_SHUTDOWN_EVENT    (1 << RSERPOOL_SHUTDOWN_EVENT)
#define NET_NOTIFICATION_MASK (NET_SESSION_CHANGE|NET_FAILOVER|NET_SHUTDOWN_EVENT)


struct NotificationNode
{
   struct NotificationNode*    Next;
   union rserpool_notification Content;
};

struct NotificationQueue
{
   struct NotificationNode* PreReadQueue;
   struct NotificationNode* PreReadLast;
   struct NotificationNode* PostReadQueue;
   struct NotificationNode* PostReadLast;
   unsigned int             EventMask;
};


void notificationQueueNew(struct NotificationQueue* notificationQueue);
void notificationQueueDelete(struct NotificationQueue* notificationQueue);
void notificationQueueClear(struct NotificationQueue* notificationQueue);
struct NotificationNode* notificationQueueEnqueueNotification(
                            struct NotificationQueue* notificationQueue,
                            const bool                isPreReadNotification,
                            const uint16_t            type);
struct NotificationNode* notificationQueueDequeueNotification(
                            struct NotificationQueue* notificationQueue,
                            const bool                fromPreReadNotifications);
bool notificationQueueHasData(struct NotificationQueue* notificationQueue);
void notificationNodeDelete(struct NotificationNode* notificationNode);


#ifdef __cplusplus
}
#endif

#endif
