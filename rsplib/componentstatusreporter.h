/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#ifndef COMPONENTSTATUSREPORTER_H
#define COMPONENTSTATUSREPORTER_H

#include "componentstatuspackets.h"
#include "rserpool.h"
#include "sockaddrunion.h"
#include "dispatcher.h"
#include "timer.h"

#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif



struct ComponentAssociation* createComponentAssociationArray(const size_t elements);
void deleteComponentAssociationArray(struct ComponentAssociation* associationArray);

void getComponentLocation(char*        componentLocation,
                          int          sd,
                          sctp_assoc_t assocID);


struct CSPReporter
{
   struct Dispatcher*   StateMachine;
   uint64_t             CSPIdentifier;
   union sockaddr_union CSPReportAddress;
   unsigned int         CSPReportInterval;
   struct Timer         CSPReportTimer;

   size_t               (*CSPGetReportFunction)(
                           void*                         userData,
                           unsigned long long*           identifier,
                           struct ComponentAssociation** caeArray,
                           char*                         statusText,
                           char*                         componentAddress,
                           double*                       workload);
   void*                CSPGetReportFunctionUserData;
};


void cspReporterNew(struct CSPReporter*    cspReporter,
                    struct Dispatcher*     dispatcher,
                    const uint64_t         cspIdentifier,
                    const struct sockaddr* cspReportAddress,
                    const unsigned int     cspReportInterval,
                    size_t                 (*cspGetReportFunction)(
                                              void*                         userData,
                                              unsigned long long*           identifier,
                                              struct ComponentAssociation** caeArray,
                                              char*                         statusText,
                                              char*                         componentAddress,
                                              double*                       workload),
                    void*                  cspGetReportFunctionUserData);
void cspReporterDelete(struct CSPReporter* cspReporter);


#ifdef __cplusplus
}
#endif

#endif
