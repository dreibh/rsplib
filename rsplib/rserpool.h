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

#ifndef RSERPOOL_H
#define RSERPOOL_H

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <ext_socket.h>
#include <netdb.h>


#ifdef __cplusplus
extern "C" {
#endif


#define RSPLIB_VERSION  0
#define RSPLIB_REVISION 9


/*
   ##########################################################################
   #### BASIC MODE                                                       ####
   ##########################################################################
*/

struct rsp_registrar_info
{
   struct rsp_registrar_info* rri_next;
   struct sockaddr*           rri_addr;
   size_t                     rri_addrs;
};

struct rsp_info
{
   unsigned int               ri_version;
   unsigned int               ri_revision;
   const char*                ri_build_date;
   const char*                ri_build_time;
   struct rsp_registrar_info* ri_registrars;

   uint64_t                   ri_csp_identifier;
   struct sockaddr*           ri_csp_server;
   unsigned int               ri_csp_interval;
};

int rsp_initialize(struct rsp_info* info);
void rsp_cleanup();


struct rsp_addrinfo {
   int                       ai_family;
   int                       ai_socktype;
   int                       ai_protocol;
   size_t                    ai_addrlen;
   size_t                    ai_addrs;
   struct sockaddr*          ai_addr;
   struct rsp_addrinfo* ai_next;
   uint32_t                  ai_pe_id;
};

struct rsp_loadinfo
{
   uint32_t rli_policy;
   uint32_t rli_weight;
   uint32_t rli_load;
   uint32_t rli_load_degradation;
};


unsigned int rsp_pe_registration(const unsigned char* poolHandle,
                                 const size_t         poolHandleSize,
                                 struct rsp_addrinfo* rspAddrInfo,
                                 struct rsp_loadinfo* rspLoadInfo,
                                 unsigned int         registrationLife);
unsigned int rsp_pe_deregistration(const unsigned char* poolHandle,
                                   const size_t         poolHandleSize,
                                   const uint32_t       identifier);
unsigned int rsp_pe_failure(const unsigned char* poolHandle,
                            const size_t         poolHandleSize,
                            const uint32_t       identifier);

int rsp_getaddrinfo(const unsigned char*  poolHandle,
                    const size_t          poolHandleSize,
                    struct rsp_addrinfo** rserpoolAddrInfo);
void rsp_freeaddrinfo(struct rsp_addrinfo* rserpoolAddrInfo);



/*
   ##########################################################################
   #### ENHANCED MODE                                                    ####
   ##########################################################################
*/


typedef unsigned int rserpool_session_t;

#define SESSION_SETSIZE 16384

#define MSG_RSERPOOL_NOTIFICATION (1 << 0)
#define MSG_RSERPOOL_COOKIE_ECHO  (1 << 1)


struct rsp_sndrcvinfo
{
   rserpool_session_t rinfo_session;
   uint32_t           rinfo_ppid;
   uint32_t           rinfo_pe_id;
   uint32_t           rinfo_timetolive;
   uint16_t           rinfo_stream;
};


struct rserpool_failover
{
   uint16_t           rf_type;
   uint16_t           rf_flags;
   uint32_t           rf_length;
   unsigned int       rf_state;
   rserpool_session_t rf_session;
   unsigned char      rf_has_cookie;
};

#define RSERPOOL_FAILOVER_NECESSARY 1
#define RSERPOOL_FAILOVER_COMPLETE  2


struct rserpool_session_change
{
   uint16_t           rsc_type;
   uint16_t           rsc_flags;
   uint32_t           rsc_length;
   unsigned int       rsc_state;
   rserpool_session_t rsc_session;
};

#define RSERPOOL_SESSION_ADD    1
#define RSERPOOL_SESSION_REMOVE 2


struct rserpool_shutdown_event
{
   uint16_t           rse_type;
   uint16_t           rse_flags;
   uint32_t           rse_length;
   rserpool_session_t rse_session;
};


union rserpool_notification
{
   struct {
      uint16_t rn_type;
      uint16_t rn_flags;
      uint32_t rn_length;
   }                              rn_header;
   struct rserpool_failover       rn_failover;
   struct rserpool_session_change rn_session_change;
   struct rserpool_shutdown_event rn_shutdown_event;
};

#define RSERPOOL_SESSION_CHANGE 1
#define RSERPOOL_FAILOVER       2
#define RSERPOOL_SHUTDOWN_EVENT 3


int rsp_socket(int domain, int type, int protocol);
int rsp_bind(int sd, struct sockaddr* addrs, int addrcnt);
int rsp_close(int sd);

int rsp_poll(struct pollfd* ufds, unsigned int nfds, int timeout);
int rsp_select(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
               struct timeval* timeout);

int rsp_mapsocket(int sd, int toSD);
int rsp_unmapsocket(int sd);


int rsp_register(int                        sd,
                 const unsigned char*       poolHandle,
                 const size_t               poolHandleSize,
                 const struct rsp_loadinfo* loadinfo,
                 unsigned int               reregistrationInterval);
int rsp_deregister(int sd);
int rsp_accept(int                sd,
               unsigned long long timeout);
int rsp_connect(int                  sd,
                const unsigned char* poolHandle,
                const size_t         poolHandleSize);
int rsp_forcefailover(int sd);


ssize_t rsp_sendmsg(int                sd,
                    const void*        data,
                    size_t             dataLength,
                    unsigned int       msg_flags,
                    rserpool_session_t sessionID,
                    uint32_t           sctpPPID,
                    uint16_t           sctpStreamID,
                    uint32_t           sctpTimeToLive,
                    unsigned long long timeout);
ssize_t rsp_recvmsg(int                    sd,
                    void*                  buffer,
                    size_t                 bufferLength,
                    struct rsp_sndrcvinfo* rinfo,
                    int*                   msg_flags,
                    unsigned long long     timeout);
ssize_t rsp_send_cookie(int                  sd,
                        const unsigned char* cookie,
                        const size_t         cookieSize,
                        rserpool_session_t   sessionID,
                        unsigned long long   timeout);
ssize_t rsp_read(int fd, void* buffer, size_t bufferLength);
ssize_t rsp_write(int fd, const char* buffer, size_t bufferLength);
ssize_t rsp_recv(int sd, void* buffer, size_t bufferLength, int flags);
ssize_t rsp_send(int sd, const void* buffer, size_t bufferLength, int flags);
int rsp_getsockopt(int sd, int level, int optname, void* optval, socklen_t* optlen);
int rsp_setsockopt(int sd, int level, int optname, const void* optval, socklen_t optlen);


void rsp_print_notification(const union rserpool_notification* notification, FILE* fd);


#ifdef __cplusplus
}
#endif

#include "rserpool-internals.h"

#endif
