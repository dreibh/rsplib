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

#ifndef RSERPOOLINTERNALS_H
#define RSERPOOLINTERNALS_H

#include "rserpool.h"


#ifdef __cplusplus
extern "C" {
#endif


struct TagItem;


#define TAG_PoolElement_Identifier                   (TAG_USER + 1000)

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


unsigned int rsp_pe_registration_tags(const unsigned char*       poolHandle,
                                      const size_t               poolHandleSize,
                                      struct rsp_addrinfo*       rspAddrInfo,
                                      const struct rsp_loadinfo* rspLoadInfo,
                                      const unsigned int         registrationLife,
                                      const int                  flags,
                                      struct TagItem*            tags);
unsigned int rsp_pe_deregistration_tags(const unsigned char* poolHandle,
                                        const size_t         poolHandleSize,
                                        const uint32_t       identifier,
                                        const int            flags,
                                        struct TagItem*      tags);
unsigned int rsp_pe_failure_tags(const unsigned char* poolHandle,
                                 const size_t         poolHandleSize,
                                 const uint32_t       identifier,
                                 struct TagItem*      tags);

int rsp_getaddrinfo_tags(const unsigned char*  poolHandle,
                         const size_t          poolHandleSize,
                         struct rsp_addrinfo** rserpoolAddrInfo,
                         const size_t          items,
                         const unsigned int    staleCacheValue,
                         struct TagItem*       tags);


int rsp_register_tags(int                        sd,
                      const unsigned char*       poolHandle,
                      const size_t               poolHandleSize,
                      const struct rsp_loadinfo* loadinfo,
                      const unsigned int         reregistrationInterval,
                      const int                  flags,
                      struct TagItem*            tags);
int rsp_deregister_tags(int            sd,
                       int             flags,
                       struct TagItem* tags);
int rsp_accept_tags(int             sd,
                    int             timeout,
                    struct TagItem* tags);
int rsp_connect_tags(int                  sd,
                     const unsigned char* poolHandle,
                     const size_t         poolHandleSize,
                     const unsigned int   staleCacheValue,
                     struct TagItem*      tags);
int rsp_forcefailover_tags(int                sd,
                           const unsigned int flags,
                           const unsigned int staleCacheValue,
                           struct TagItem*    tags);


#ifdef __cplusplus
}
#endif

#endif
