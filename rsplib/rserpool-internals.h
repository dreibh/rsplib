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


#ifdef __cplusplus
extern "C" {
#endif


struct TagItem;


#define TAG_PoolElement_Identifier                   (TAG_USER + 1000)
#define TAG_UserTransport_HasControlChannel          (TAG_USER + 1050)

#define TAG_RspSession_ConnectTimeout                (TAG_USER + 2000)
#define TAG_RspSession_HandleResolutionRetryDelay    (TAG_USER + 2001)

#define TAG_RspLib_CacheElementTimeout               (TAG_USER + 4000)
#define TAG_RspLib_RegistrarAnnounceAddress          (TAG_USER + 4001)
#define TAG_RspLib_RegistrarAnnounceTimeout          (TAG_USER + 4002)
#define TAG_RspLib_RegistrarConnectMaxTrials         (TAG_USER + 4003)
#define TAG_RspLib_RegistrarConnectTimeout           (TAG_USER + 4004)
#define TAG_RspLib_RegistrarRequestMaxTrials         (TAG_USER + 4005)
#define TAG_RspLib_RegistrarRequestTimeout           (TAG_USER + 4006)
#define TAG_RspLib_RegistrarResponseTimeout          (TAG_USER + 4007)

#define TAG_RspPERegistration_WaitForResult          (TAG_USER + 5000)
#define TAG_RspPEDeregistration_WaitForResult        (TAG_USER + 5001)


unsigned int rsp_pe_registration_tags(const unsigned char* poolHandle,
                                      const size_t         poolHandleSize,
                                      struct rsp_addrinfo* rspAddrInfo,
                                      struct rsp_loadinfo* rspLoadInfo,
                                      unsigned int         registrationLife,
                                      struct TagItem*      tags);
unsigned int rsp_pe_deregistration_tags(const unsigned char* poolHandle,
                                        const size_t         poolHandleSize,
                                        const uint32_t       identifier,
                                        struct TagItem*      tags);
unsigned int rsp_pe_failure_tags(const unsigned char* poolHandle,
                                 const size_t         poolHandleSize,
                                 const uint32_t       identifier,
                                 struct TagItem*      tags);

int rsp_getaddrinfo_tags(const unsigned char*  poolHandle,
                         const size_t          poolHandleSize,
                         struct rsp_addrinfo** rserpoolAddrInfo,
                         struct TagItem*       tags);


int rsp_register_tags(int                        sd,
                      const unsigned char*       poolHandle,
                      const size_t               poolHandleSize,
                      const struct rsp_loadinfo* loadinfo,
                      unsigned int               reregistrationInterval,
                      struct TagItem*            tags);
int rsp_deregister_tags(int sd, struct TagItem* tags);
int rsp_accept_tags(int                sd,
                    unsigned long long timeout,
                    struct TagItem*    tags);
int rsp_connect_tags(int                  sd,
                     const unsigned char* poolHandle,
                     const size_t         poolHandleSize,
                     struct TagItem*      tags);
int rsp_forcefailover_tags(int             sd,
                           struct TagItem* tags);


/**
  * Set session's application status text for rsp_csp_getcomponentstatus().
  *
  * @param sd SessionDescriptor.
  * @param statusText Status text.
  */
int rsp_csp_setstatus(int                sd,
                      rserpool_session_t sessionID,
                      const char*        statusText);

#ifdef __cplusplus
}
#endif

#endif
