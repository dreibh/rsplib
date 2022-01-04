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
 * Copyright (C) 2002-2022 by Thomas Dreibholz
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

#ifndef INTERTHREADMESSAGEPORT_H
#define INTERTHREADMESSAGEPORT_H

#include "tdtypes.h"
#include "threadsignal.h"
#include "doublelinkedringlist.h"


#ifdef __cplusplus
extern "C" {
#endif


struct InterThreadMessagePort;

struct InterThreadMessageNode
{
   struct DoubleLinkedRingListNode Node;
   struct InterThreadMessagePort*  ReplyPort;
};

struct InterThreadMessagePort
{
   struct DoubleLinkedRingList Queue;
   struct ThreadSignal         Signal;
};


void interThreadMessagePortNew(struct InterThreadMessagePort* itmPort);
void interThreadMessagePortDelete(struct InterThreadMessagePort* itmPort);
void interThreadMessagePortLock(struct InterThreadMessagePort* itmPort);
void interThreadMessagePortUnlock(struct InterThreadMessagePort* itmPort);
bool interThreadMessagePortIsFirstMessage(struct InterThreadMessagePort* itmPort,
                                          struct InterThreadMessageNode* message);
struct InterThreadMessageNode* interThreadMessagePortGetFirstMessage(struct InterThreadMessagePort* itmPort);
struct InterThreadMessageNode* interThreadMessagePortGetNextMessage(struct InterThreadMessagePort* itmPort,
                                                                    struct InterThreadMessageNode* message);
void interThreadMessagePortRemoveMessage(struct InterThreadMessagePort* itmPort,
                                         struct InterThreadMessageNode* message);
void interThreadMessagePortEnqueue(struct InterThreadMessagePort* itmPort,
                                   struct InterThreadMessageNode* message,
                                   struct InterThreadMessagePort* replyPort);
struct InterThreadMessageNode* interThreadMessagePortDequeue(struct InterThreadMessagePort* itmPort);
void interThreadMessagePortWait(struct InterThreadMessagePort* itmPort);
void interThreadMessageReply(struct InterThreadMessageNode* message);


#ifdef __cplusplus
}
#endif

#endif
