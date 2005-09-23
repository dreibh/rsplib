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

#ifndef RSERPOOLINTERNALS_H
#define RSERPOOLINTERNALS_H

#include "rserpool.h"
#include "poolpolicysettings.h"
#include "componentstatusprotocol.h"

#ifdef __cplusplus
extern "C" {
#endif


#define TAG_PoolElement_ReregistrationInterval       (TAG_USER + 1000)
#define TAG_PoolElement_RegistrationLife             (TAG_USER + 1001)
#define TAG_PoolElement_Identifier                   (TAG_USER + 1002)
#define TAG_UserTransport_HasControlChannel          (TAG_USER + 1050)

#define TAG_RspSession_ConnectTimeout                (TAG_USER + 2000)
#define TAG_RspSession_HandleResolutionRetryDelay    (TAG_USER + 2001)

#define TAG_PoolPolicy_Type                          (TAG_USER + 3000)
#define TAG_PoolPolicy_Parameter_Weight              (TAG_USER + 3001)
#define TAG_PoolPolicy_Parameter_Load                (TAG_USER + 3002)
#define TAG_PoolPolicy_Parameter_LoadDegradation     (TAG_USER + 3003)

#define TAG_RspLib_CacheElementTimeout               (TAG_USER + 4000)
#define TAG_RspLib_RegistrarAnnounceAddress          (TAG_USER + 4001)
#define TAG_RspLib_RegistrarAnnounceTimeout          (TAG_USER + 4002)
#define TAG_RspLib_RegistrarConnectMaxTrials         (TAG_USER + 4003)
#define TAG_RspLib_RegistrarConnectTimeout           (TAG_USER + 4004)
#define TAG_RspLib_RegistrarRequestMaxTrials         (TAG_USER + 4005)
#define TAG_RspLib_RegistrarRequestTimeout           (TAG_USER + 4006)
#define TAG_RspLib_RegistrarResponseTimeout          (TAG_USER + 4007)



/**
  * Set session's application status text for rsp_csp_getcomponentstatus().
  *
  * @param sd SessionDescriptor.
  * @param statusText Status text.
  */
int rsp_csp_setstatus(int                sd,
                      rserpool_session_t sessionID,
                      const char*        statusText);

/**
  * Get component status. The resulting status is dynamically allocated and has to
  * be freed by the caller!
  *
  * @param caeArray Reference to store pointer to resulting ComponentAssociationEntry array to.
  * @param statusText Reference of buffer to store status text to.
  * @param componentAddress Reference of buffer to store component address to.
  * @return Number of ComponentAssociationEntries created.
*/
size_t rsp_csp_getcomponentstatus(struct ComponentAssociationEntry** caeArray,
                                  char*                              statusText,
                                  char*                              componentAddress);


#ifdef __cplusplus
}
#endif

#endif
