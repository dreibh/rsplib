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

#ifndef TAKEOVERPROCESS_H
#define TAKEOVERPROCESS_H

#include "poolhandlespacemanagement.h"

#ifdef __cplusplus
extern "C" {
#endif


struct TakeoverProcess
{
   struct STN_CLASSNAME    IndexStorageNode;
   struct STN_CLASSNAME    TimerStorageNode;

   RegistrarIdentifierType TargetID;
   unsigned long long      ExpiryTimeStamp;

   size_t                  OutstandingAcknowledgements;
   RegistrarIdentifierType PeerIDArray[0];
};


struct TakeoverProcess* getTakeoverProcessFromIndexStorageNode(struct STN_CLASSNAME* node);
struct TakeoverProcess* getTakeoverProcessFromTimerStorageNode(struct STN_CLASSNAME* node);
void takeoverProcessIndexPrint(const void* takeoverProcessPtr,
                               FILE*       fd);
int takeoverProcessIndexComparison(const void* takeoverProcessPtr1,
                                   const void* takeoverProcessPtr2);
int takeoverProcessTimerComparison(const void* takeoverProcessPtr1,
                                   const void* takeoverProcessPtr2);


#ifdef __cplusplus
}
#endif

#endif
