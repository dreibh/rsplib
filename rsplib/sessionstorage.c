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
 * Copyright (C) 2002-2009 by Thomas Dreibholz
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

#include "tdtypes.h"
#include "sessionstorage.h"
#include "debug.h"

#include <stdio.h>


/* ###### Get session from assoc ID storage node ######################### */
static struct Session* getSessionFromAssocIDStorageNode(void* node)
{
   return((struct Session*)((long)node -
             ((long)&((struct Session *)node)->AssocIDNode - (long)node)));
}


/* ###### Get session from session ID storage node ####################### */
static struct Session* getSessionFromSessionIDStorageNode(void* node)
{
   return((struct Session*)((long)node -
             ((long)&((struct Session *)node)->SessionIDNode - (long)node)));
}


/* ###### Print assoc ID storage node #################################### */
static void sessionAssocIDPrint(const void* node, FILE* fd)
{
   const struct Session* session = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node);

   fprintf(fd, "A%u[S%u] ", (unsigned int)session->AssocID, session->SessionID);
}


/* ###### Compare assoc ID storage nodes ################################# */
static int sessionAssocIDComparison(const void* node1, const void* node2)
{
   const struct Session* session1 = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node1);
   const struct Session* session2 = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node2);

   if(session1->AssocID < session2->AssocID) {
      return(-1);
   }
   else if(session1->AssocID > session2->AssocID) {
      return(1);
   }
   return(0);
}


/* ###### Print session ID storage node ################################## */
static void sessionSessionIDPrint(const void* node, FILE* fd)
{
   const struct Session* session = (const struct Session*)getSessionFromSessionIDStorageNode((void*)node);

   fprintf(fd, "%u[A%u] ", session->SessionID, (unsigned int)session->AssocID);
}


/* ###### Compare session ID storage nodes ############################### */
static int sessionSessionIDComparison(const void* node1, const void* node2)
{
   const struct Session* session1 = (const struct Session*)getSessionFromSessionIDStorageNode((void*)node1);
   const struct Session* session2 = (const struct Session*)getSessionFromSessionIDStorageNode((void*)node2);

   if(session1->SessionID < session2->SessionID) {
      return(-1);
   }
   else if(session1->SessionID > session2->SessionID) {
      return(1);
   }
   return(0);
}


/* ###### Constructor #################################################### */
void sessionStorageNew(struct SessionStorage* sessionStorage)
{
   simpleRedBlackTreeNew(&sessionStorage->AssocIDSet, sessionAssocIDPrint, sessionAssocIDComparison);
   simpleRedBlackTreeNew(&sessionStorage->SessionIDSet, sessionSessionIDPrint, sessionSessionIDComparison);
}


/* ###### Destructor ##################################################### */
void sessionStorageDelete(struct SessionStorage* sessionStorage)
{
   CHECK(simpleRedBlackTreeIsEmpty(&sessionStorage->AssocIDSet));
   CHECK(simpleRedBlackTreeIsEmpty(&sessionStorage->SessionIDSet));
   simpleRedBlackTreeDelete(&sessionStorage->AssocIDSet);
   simpleRedBlackTreeDelete(&sessionStorage->SessionIDSet);
}


/* ###### Add session #################################################### */
void sessionStorageAddSession(struct SessionStorage* sessionStorage,
                              struct Session*        session)
{
   CHECK(simpleRedBlackTreeInsert(&sessionStorage->SessionIDSet, &session->SessionIDNode) == &session->SessionIDNode);
   CHECK(simpleRedBlackTreeInsert(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Delete session ################################################# */
void sessionStorageDeleteSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session)
{
   CHECK(simpleRedBlackTreeRemove(&sessionStorage->SessionIDSet, &session->SessionIDNode) == &session->SessionIDNode);
   CHECK(simpleRedBlackTreeRemove(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Update session's assoc ID ###################################### */
void sessionStorageUpdateSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session,
                                 sctp_assoc_t           newAssocID)
{
   CHECK(simpleRedBlackTreeRemove(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
   session->AssocID = newAssocID;
   CHECK(simpleRedBlackTreeInsert(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Is session storage empty? ###################################### */
bool sessionStorageIsEmpty(struct SessionStorage* sessionStorage)
{
   return(simpleRedBlackTreeIsEmpty(&sessionStorage->SessionIDSet));
}


/* ###### Get number of sessions ######################################### */
size_t sessionStorageGetElements(struct SessionStorage* sessionStorage)
{
   return(simpleRedBlackTreeGetElements(&sessionStorage->SessionIDSet));
}


/* ###### Print session storage ########################################## */
void sessionStoragePrint(struct SessionStorage* sessionStorage,
                         FILE*                  fd)
{
   fputs("SessionStorage:\n", fd);
   fputs(" by Session ID: ", fd);
   simpleRedBlackTreePrint(&sessionStorage->SessionIDSet, fd);
   fputs(" by Assoc ID:   ", fd);
   simpleRedBlackTreePrint(&sessionStorage->AssocIDSet, fd);
}


/* ###### Find session by session ID ##################################### */
struct Session* sessionStorageFindSessionBySessionID(struct SessionStorage* sessionStorage,
                                                     rserpool_session_t     sessionID)
{
   struct Session                     cmpNode;
   struct SimpleRedBlackTreeNode* node;

   cmpNode.SessionID = sessionID;

   node = simpleRedBlackTreeFind(&sessionStorage->SessionIDSet, &cmpNode.SessionIDNode);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Find session by assoc ID ####################################### */
struct Session* sessionStorageFindSessionByAssocID(struct SessionStorage* sessionStorage,
                                                   sctp_assoc_t           assocID)
{
   struct Session                     cmpNode;
   struct SimpleRedBlackTreeNode* node;

   cmpNode.AssocID = assocID;

   node = simpleRedBlackTreeFind(&sessionStorage->AssocIDSet, &cmpNode.AssocIDNode);
   if(node != NULL) {
      return(getSessionFromAssocIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Get first session ############################################## */
struct Session* sessionStorageGetFirstSession(struct SessionStorage* sessionStorage)
{
   struct SimpleRedBlackTreeNode* node;
   node = simpleRedBlackTreeGetFirst(&sessionStorage->SessionIDSet);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Get next session ############################################### */
struct Session* sessionStorageGetNextSession(struct SessionStorage* sessionStorage,
                                             struct Session*        session)
{
   struct SimpleRedBlackTreeNode* node;
   node = simpleRedBlackTreeGetNext(&sessionStorage->SessionIDSet, &session->SessionIDNode);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}
