/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#ifndef NOTIFICATIONQUEUE_H
#define NOTIFICATIONQUEUE_H

#include "tdtypes.h"
#include "rserpool.h"


#ifdef __cplusplus
extern "C" {
#endif


#define NT_SESSION_CHANGE    (1 << RSERPOOL_SESSION_CHANGE)
#define NT_FAILOVER          (1 << RSERPOOL_FAILOVER)
#define NT_SHUTDOWN_EVENT    (1 << RSERPOOL_SHUTDOWN_EVENT)
#define NT_NOTIFICATION_MASK (NT_SESSION_CHANGE|NT_FAILOVER|NT_SHUTDOWN_EVENT)


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
};


void notificationQueueNew(struct NotificationQueue* notificationQueue);
void notificationQueueDelete(struct NotificationQueue* notificationQueue);
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
