/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#ifndef SESSIONSTORAGE_H
#define SESSIONSTORAGE_H

#include "tdtypes.h"
#include "session.h"
#include "simpleredblacktree.h"

#ifdef __cplusplus
extern "C" {
#endif


struct SessionStorage
{
   struct SimpleRedBlackTree AssocIDSet;
   struct SimpleRedBlackTree SessionIDSet;
};


void sessionStorageNew(struct SessionStorage* sessionStorage);
void sessionStorageDelete(struct SessionStorage* sessionStorage);
void sessionStorageAddSession(struct SessionStorage* sessionStorage,
                              struct Session*        session);
void sessionStorageDeleteSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session);
void sessionStorageUpdateSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session,
                                 sctp_assoc_t           newAssocID);
bool sessionStorageIsEmpty(struct SessionStorage* sessionStorage);
size_t sessionStorageGetElements(struct SessionStorage* sessionStorage);
void sessionStoragePrint(struct SessionStorage* sessionStorage,
                         FILE*                  fd);
struct Session* sessionStorageFindSessionBySessionID(struct SessionStorage* sessionStorage,
                                                     rserpool_session_t     sessionID);
struct Session* sessionStorageFindSessionByAssocID(struct SessionStorage* sessionStorage,
                                                   sctp_assoc_t           assocID);
struct Session* sessionStorageGetFirstSession(struct SessionStorage* sessionStorage);
struct Session* sessionStorageGetNextSession(struct SessionStorage* sessionStorage,
                                             struct Session*        session);


#ifdef __cplusplus
}
#endif

#endif
