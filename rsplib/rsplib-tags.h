/*
 *  $Id: rsplib-tags.h,v 1.6 2004/09/16 16:24:43 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: RSerPool Library Tag Item Space
 *
 */


#ifndef RSPLIBTAGS_H
#define RSPLIBTAGS_H


#include "tagitem.h"


#define TAG_RspLib_GetVersion        (TAG_USER + 10000)
#define TAG_RspLib_GetRevision       (TAG_USER + 10001)
#define TAG_RspLib_GetBuildDate      (TAG_USER + 10002)
#define TAG_RspLib_GetBuildTime      (TAG_USER + 10003)
#define TAG_RspLib_IsThreadSafe      (TAG_USER + 10004)

#define TAG_RspLib_CSPIdentifier     (TAG_USER + 10010)
#define TAG_RspLib_CSPReportAddress  (TAG_USER + 10011)
#define TAG_RspLib_CSPReportInterval (TAG_USER + 10012)

#define TAG_RspLib_CacheElementTimeout                   (TAG_USER + 7000)
#define TAG_RspLib_NameServerAnnounceAddress             (TAG_USER + 7001)
#define TAG_RspLib_NameServerAnnounceTimeout             (TAG_USER + 7002)
#define TAG_RspLib_NameServerConnectMaxTrials            (TAG_USER + 7003)
#define TAG_RspLib_NameServerConnectTimeout              (TAG_USER + 7004)
#define TAG_RspLib_NameServerRequestMaxTrials            (TAG_USER + 7005)
#define TAG_RspLib_NameServerRequestTimeout              (TAG_USER + 7006)
#define TAG_RspLib_NameServerResponseTimeout             (TAG_USER + 7007)

#define TAG_PoolPolicy_Type                          (TAG_USER + 1000)

#define TAG_PoolPolicy_Parameter_Weight              (TAG_USER + 1001)
#define TAG_PoolPolicy_Parameter_Load                (TAG_USER + 1002)
#define TAG_PoolPolicy_Parameter_LoadDegradation     (TAG_USER + 1003)

#define TAG_PoolElement_ReregistrationInterval (TAG_USER + 2000)
#define TAG_PoolElement_RegistrationLife       (TAG_USER + 2001)
#define TAG_PoolElement_SocketDomain           (TAG_USER + 2002)
#define TAG_PoolElement_SocketType             (TAG_USER + 2003)
#define TAG_PoolElement_SocketProtocol         (TAG_USER + 2004)
#define TAG_PoolElement_LocalPort              (TAG_USER + 2005)
#define TAG_PoolElement_Identifier             (TAG_USER + 2006)

#define TAG_RspIO_Flags           (TAG_USER + 8000)
#define TAG_RspIO_Timeout         (TAG_USER + 8001)
#define TAG_RspIO_PE_ID           (TAG_USER + 8002)
#define TAG_RspIO_SCTP_AssocID    (TAG_USER + 8003)
#define TAG_RspIO_SCTP_StreamID   (TAG_USER + 8004)
#define TAG_RspIO_SCTP_PPID       (TAG_USER + 8005)
#define TAG_RspIO_SCTP_TimeToLive (TAG_USER + 8006)
#define TAG_RspIO_MakeFailover    (TAG_USER + 8007)
#define TAG_RspIO_MsgIsCookie     (TAG_USER + 8008)

#define TAG_RspSession_ConnectTimeout           (TAG_USER + 7000)
#define TAG_RspSession_NameResolutionRetryDelay (TAG_USER + 7001)

#define TAG_RspSession_ReceivedCookieEchoCallback (TAG_USER + 7003)
#define TAG_RspSession_ReceivedCookieEchoUserData (TAG_USER + 7004)
#define TAG_RspSession_FailoverCallback           (TAG_USER + 7005)
#define TAG_RspSession_FailoverUserData           (TAG_USER + 7006)

#define TAG_UserTransport_HasControlChannel       (TAG_USER + 7500)


typedef void (*CookieEchoCallbackPtr)(void* userData, void* cookieData, const size_t cookieSize);
typedef bool (*FailoverCallbackPtr)(void* userData);


#define TAG_RspSelect_MaxFD           (TAG_USER + 9000)
#define TAG_RspSelect_ReadFDs         (TAG_USER + 9001)
#define TAG_RspSelect_WriteFDs        (TAG_USER + 9002)
#define TAG_RspSelect_ExceptFDs       (TAG_USER + 9003)
#define TAG_RspSelect_Timeout         (TAG_USER + 9004)
#define TAG_RspSelect_RsplibEventLoop (TAG_USER + 9005)


#define RspRead_ReadError   -1
#define RspRead_WrongPPID   -2
#define RspRead_PartialRead -3
#define RspRead_MessageRead -4
#define RspRead_TooBig      -5
#define RspRead_Timeout     -6


#endif
