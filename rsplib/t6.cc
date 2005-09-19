#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <ext_socket.h>
#include "randomizer.h"
#include "statistics.h"
#include "debug.h"
#include "leaflinkedredblacktree.h"


typedef unsigned int rserpool_session_t;


struct SessionStorage
{
   struct LeafLinkedRedBlackTree AssocIDSet;
   struct LeafLinkedRedBlackTree SessionIDSet;
};

struct Session
{
   struct LeafLinkedRedBlackTreeNode SessionIDNode;
   struct LeafLinkedRedBlackTreeNode AssocIDNode;

   sctp_assoc_t                      AssocID;
   rserpool_session_t                SessionID;
};


/* ###### Get session from assoc ID storage node ######################### */
inline static struct Session* getSessionFromAssocIDStorageNode(void* node)
{
   return((struct Session*)((long)node -
             ((long)&((struct Session *)node)->AssocIDNode - (long)node)));
}


/* ###### Get session from session ID storage node ####################### */
inline static struct Session* getSessionFromSessionIDStorageNode(void* node)
{
   return((struct Session*)((long)node -
             ((long)&((struct Session *)node)->SessionIDNode - (long)node)));
}


/* ###### Print assoc ID storage node #################################### */
static void sessionAssocIDPrint(const void* node, FILE* fd)
{
   const struct Session* session = (const struct Session*)getSessionFromAssocIDStorageNode((void*)node);

   fprintf(fd, "A%u[S%u] ", session->AssocID, session->SessionID);
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
   leafLinkedRedBlackTreeNew(&sessionStorage->AssocIDSet, sessionAssocIDPrint, sessionAssocIDComparison);
   leafLinkedRedBlackTreeNew(&sessionStorage->SessionIDSet, sessionSessionIDPrint, sessionSessionIDComparison);
}


/* ###### Destructor ##################################################### */
void sessionStorageDelete(struct SessionStorage* sessionStorage)
{
   CHECK(leafLinkedRedBlackTreeIsEmpty(&sessionStorage->AssocIDSet));
   CHECK(leafLinkedRedBlackTreeIsEmpty(&sessionStorage->SessionIDSet));
   leafLinkedRedBlackTreeDelete(&sessionStorage->AssocIDSet);
   leafLinkedRedBlackTreeDelete(&sessionStorage->SessionIDSet);
}


/* ###### Add session #################################################### */
void sessionStorageAddSession(struct SessionStorage* sessionStorage,
                              struct Session*        session)
{
   CHECK(leafLinkedRedBlackTreeInsert(&sessionStorage->SessionIDSet, &session->SessionIDNode) == &session->SessionIDNode);
   CHECK(leafLinkedRedBlackTreeInsert(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Delete session ################################################# */
void sessionStorageDeleteSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session)
{
   CHECK(leafLinkedRedBlackTreeRemove(&sessionStorage->SessionIDSet, &session->SessionIDNode) == &session->SessionIDNode);
   CHECK(leafLinkedRedBlackTreeRemove(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Update session's assoc ID ###################################### */
void sessionStorageUpdateSession(struct SessionStorage* sessionStorage,
                                 struct Session*        session,
                                 sctp_assoc_t           newAssocID)
{
   CHECK(leafLinkedRedBlackTreeRemove(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
   session->AssocID = newAssocID;
   CHECK(leafLinkedRedBlackTreeInsert(&sessionStorage->AssocIDSet, &session->AssocIDNode) == &session->AssocIDNode);
}


/* ###### Is session storage empty? ###################################### */
bool sessionStorageIsEmpty(struct SessionStorage* sessionStorage)
{
   return(leafLinkedRedBlackTreeIsEmpty(&sessionStorage->SessionIDSet));
}


/* ###### Get number of sessions ######################################### */
size_t sessionStorageGetElements(struct SessionStorage* sessionStorage)
{
   return(leafLinkedRedBlackTreeGetElements(&sessionStorage->SessionIDSet));
}


/* ###### Print session storage ########################################## */
void sessionStoragePrint(struct SessionStorage* sessionStorage,
                         FILE*                  fd)
{
   fputs("SessionStorage:\n", fd);
   fputs(" by Session ID: ", fd);
   leafLinkedRedBlackTreePrint(&sessionStorage->SessionIDSet, fd);
   fputs(" by Assoc ID:   ", fd);
   leafLinkedRedBlackTreePrint(&sessionStorage->AssocIDSet, fd);
}


/* ###### Find session by session ID ##################################### */
struct Session* sessionStorageFindSessionBySessionID(struct SessionStorage* sessionStorage,
                                                     rserpool_session_t     sessionID)
{
   struct Session                     cmpNode;
   struct LeafLinkedRedBlackTreeNode* node;

   cmpNode.SessionID = sessionID;

   node = leafLinkedRedBlackTreeFind(&sessionStorage->SessionIDSet, &cmpNode.SessionIDNode);
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
   struct LeafLinkedRedBlackTreeNode* node;

   cmpNode.AssocID = assocID;

   node = leafLinkedRedBlackTreeFind(&sessionStorage->AssocIDSet, &cmpNode.AssocIDNode);
   if(node != NULL) {
      return(getSessionFromAssocIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Get first session ############################################## */
struct Session* sessionStorageGetFirstSession(struct SessionStorage* sessionStorage)
{
   struct LeafLinkedRedBlackTreeNode* node;
   node = leafLinkedRedBlackTreeGetFirst(&sessionStorage->SessionIDSet);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}


/* ###### Get next session ############################################### */
struct Session* sessionStorageGetNextSession(struct SessionStorage* sessionStorage,
                                             struct Session*        session)
{
   struct LeafLinkedRedBlackTreeNode* node;
   node = leafLinkedRedBlackTreeGetNext(&sessionStorage->SessionIDSet, &session->SessionIDNode);
   if(node != NULL) {
      return(getSessionFromSessionIDStorageNode(node));
   }
   return(NULL);
}



int main(int argc, char** argv)
{
   SessionStorage ss;
   sessionStorageNew(&ss);

   Session s1;
   Session s2;
   s1.SessionID = 1;
   s1.AssocID = 0;
   s2.SessionID = 2;
   s2.AssocID = 1;

   sessionStorageAddSession(&ss, &s1);
   sessionStorageAddSession(&ss, &s2);

   sessionStoragePrint(&ss, stdout);

   CHECK(sessionStorageFindSessionBySessionID(&ss, 1) == &s1);
   CHECK(sessionStorageFindSessionBySessionID(&ss, 2) == &s2);
   CHECK(sessionStorageFindSessionByAssocID(&ss, 0) == &s1);
   CHECK(sessionStorageFindSessionByAssocID(&ss, 1) == &s2);

   sessionStorageUpdateSession(&ss, &s1, 9999);

   CHECK(sessionStorageFindSessionBySessionID(&ss, 1) == &s1);
   CHECK(sessionStorageFindSessionBySessionID(&ss, 2) == &s2);
   CHECK(sessionStorageFindSessionByAssocID(&ss, 9999) == &s1);
   CHECK(sessionStorageFindSessionByAssocID(&ss, 1) == &s2);

   sessionStoragePrint(&ss, stdout);

   sessionStorageDeleteSession(&ss, &s1);
   sessionStorageDeleteSession(&ss, &s2);

   sessionStorageDelete(&ss);
   return 0;
}
