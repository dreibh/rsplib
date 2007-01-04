/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef TAKEOVERPROCESSLIST_H
#define TAKEOVERPROCESSLIST_H

#include "poolhandlespacemanagement.h"
#include "takeoverprocess.h"

#ifdef __cplusplus
extern "C" {
#endif


struct TakeoverProcessList
{
   struct ST_CLASSNAME TakeoverProcessIndexStorage;
   struct ST_CLASSNAME TakeoverProcessTimerStorage;
};


void takeoverProcessListNew(struct TakeoverProcessList* takeoverProcessList);
void takeoverProcessListDelete(struct TakeoverProcessList* takeoverProcessList);
void takeoverProcessListPrint(struct TakeoverProcessList* takeoverProcessList,
                              FILE*                       fd);
unsigned long long takeoverProcessListGetNextTimerTimeStamp(
                      struct TakeoverProcessList* takeoverProcessList);
struct TakeoverProcess* takeoverProcessListGetEarliestTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList);
struct TakeoverProcess* takeoverProcessListGetFirstTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList);
struct TakeoverProcess* takeoverProcessListGetLastTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList);
struct TakeoverProcess* takeoverProcessListGetNextTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList,
                                  struct TakeoverProcess*     takeoverProcess);
struct TakeoverProcess* takeoverProcessListGetPrevTakeoverProcess(
                                  struct TakeoverProcessList* takeoverProcessList,
                                  struct TakeoverProcess*     takeoverProcess);
struct TakeoverProcess* takeoverProcessListFindTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList,
                           const RegistrarIdentifierType    targetID);
unsigned int takeoverProcessListCreateTakeoverProcess(
                struct TakeoverProcessList*          takeoverProcessList,
                const RegistrarIdentifierType        targetID,
                struct ST_CLASS(PeerListManagement)* peerList,
                const unsigned long long             expiryTimeStamp);
struct TakeoverProcess* takeoverProcessListAcknowledgeTakeoverProcess(
                           struct TakeoverProcessList* takeoverProcessList,
                           const RegistrarIdentifierType    targetID,
                           const RegistrarIdentifierType    acknowledgerID);
void takeoverProcessListDeleteTakeoverProcess(
        struct TakeoverProcessList* takeoverProcessList,
        struct TakeoverProcess*     takeoverProcess);


#ifdef __cplusplus
}
#endif

#endif
