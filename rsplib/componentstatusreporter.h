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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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
                           uint64_t*                     identifier,
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
                                              uint64_t*                     identifier,
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
