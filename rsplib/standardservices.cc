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

#include "standardservices.h"


/*
   ##########################################################################
   #### ECHO                                                             ####
   ##########################################################################
*/

// ###### Constructor #######################################################
EchoServer::EchoServer()
{
}


// ###### Destructor ########################################################
EchoServer::~EchoServer()
{
}


// ###### Handle message ####################################################
EventHandlingResult EchoServer::handleMessage(rserpool_session_t sessionID,
                                              const char*        buffer,
                                              size_t             bufferSize,
                                              uint32_t           ppid,
                                              uint16_t           streamID)
{
   ssize_t sent;
   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                      buffer, bufferSize, 0,
                      sessionID, ppid, streamID,
                      0, 0);
   printf("snd=%d\n", sent);
   return((sent == (ssize_t)bufferSize) ? EHR_Okay : EHR_Abort);
}
