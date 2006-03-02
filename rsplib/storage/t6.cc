#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <ext_socket.h>

#include "rserpool.h"
#include "session.h"
#include "debug.h"


#define NT_SESSION_CHANGE    (1 << RSERPOOL_SESSION_CHANGE)
#define NT_FAILOVER          (1 << RSERPOOL_FAILOVER)
#define NT_NOTIFICATION_MASK (NT_SESSION_CHANGE|NT_FAILOVER)


struct NotificationNode
{
   struct NotificationNode*    Next;
   union rserpool_notification Content;
};

struct NotificationQueue
{
   struct NotificationNode* PreReadQueue;
   struct NotificationNode* PreReadLast;
   struct NotificationNode* PostReadQueue;
   struct NotificationNode* PostReadLast;
   unsigned int             EventMask;
};


/* ###### Constructor #################################################### */
void notificationQueueNew(struct NotificationQueue* notificationQueue)
{
   notificationQueue->PreReadQueue  = NULL;
   notificationQueue->PreReadLast   = NULL;
   notificationQueue->PostReadQueue = NULL;
   notificationQueue->PostReadLast  = NULL;
   notificationQueue->EventMask     = 0;
}


/* ###### Destructor ##################################################### */
void notificationQueueDelete(struct NotificationQueue* notificationQueue)
{
   struct NotificationNode* next;

   while(notificationQueue->PreReadQueue) {
      next = notificationQueue->PreReadQueue->Next;
      free(notificationQueue->PreReadQueue);
      notificationQueue->PreReadQueue = next;
   }
   notificationQueue->PreReadLast = NULL;

   while(notificationQueue->PostReadQueue) {
      next = notificationQueue->PostReadQueue->Next;
      free(notificationQueue->PostReadQueue);
      notificationQueue->PostReadQueue = next;
   }
   notificationQueue->PostReadLast = NULL;
}


/* ###### Enqueue notification ########################################### */
struct NotificationNode* notificationQueueEnqueueNotification(
                            struct NotificationQueue* notificationQueue,
                            const bool                isPreReadNotification,
                            const uint16_t            type)
{
   struct NotificationNode* notificationNode;

   notificationNode = (struct NotificationNode*)malloc(sizeof(struct NotificationNode));
   if(notificationNode) {
      /* ====== Set pending events appropriately ========================= */
      notificationNode->Content.rn_header.rn_type = type;

      /* ====== Add notification node ==================================== */
      notificationNode->Next = NULL;
      if(isPreReadNotification) {
          if(notificationQueue->PreReadLast) {
             notificationQueue->PreReadLast->Next = notificationNode;
          }
          else {
             notificationQueue->PreReadQueue = notificationNode;
          }
          notificationQueue->PreReadLast = notificationNode;
      }
      else {
          if(notificationQueue->PostReadLast) {
             notificationQueue->PostReadLast->Next = notificationNode;
          }
          else {
             notificationQueue->PostReadQueue = notificationNode;
          }
          notificationQueue->PostReadLast = notificationNode;
      }
   }
   return(notificationNode);
}


/* ###### Dequeue notification ########################################### */
struct NotificationNode* notificationQueueDequeueNotification(
                            struct NotificationQueue* notificationQueue,
                            const bool                fromPreReadNotifications)
{
   struct NotificationNode* notificationNode;

   if(fromPreReadNotifications) {
      notificationNode = notificationQueue->PreReadQueue;
      if(notificationNode) {
         notificationQueue->PreReadQueue = notificationNode->Next;
      }
      if(notificationNode == notificationQueue->PreReadLast) {
         notificationQueue->PreReadLast = NULL;
      }
   }
   else {
      notificationNode = notificationQueue->PostReadQueue;
      if(notificationNode) {
         notificationQueue->PostReadQueue = notificationNode->Next;
      }
      if(notificationNode == notificationQueue->PostReadLast) {
         notificationQueue->PostReadLast = NULL;
      }
   }
   return(notificationNode);
}


/* ###### Destructor ##################################################### */
void notificationNodeDelete(struct NotificationNode* notificationNode)
{
   free(notificationNode);
}




//             notification->rn_session_change.rsc_type    = RSERPOOL_SESSION_CHANGE;
//             notification->rn_session_change.rsc_flags   = 0x00;
//             notification->rn_session_change.rsc_length  = sizeof(notification);
//             notification->rn_session_change.rsc_state   = RSERPOOL_SESSION_ADD;
//             notification->rn_session_change.rsc_session = session->SessionID;

int main(int argc, char** argv)
{
   struct NotificationQueue nq;

   notificationQueueNew(&nq);

   bool pr=false;

   struct NotificationNode* n1 = notificationQueueEnqueueNotification(&nq, pr, NT_SESSION_CHANGE);
   struct NotificationNode* n2 = notificationQueueEnqueueNotification(&nq, pr, NT_SESSION_CHANGE);
   struct NotificationNode* n3 = notificationQueueEnqueueNotification(&nq, pr, NT_FAILOVER);
   struct NotificationNode* n4 = notificationQueueEnqueueNotification(&nq, pr, NT_SESSION_CHANGE);
   struct NotificationNode* n5 = notificationQueueEnqueueNotification(&nq, pr, NT_SESSION_CHANGE);
   n1->Content.rn_header.rn_flags=1;
   n2->Content.rn_header.rn_flags=2;
   n3->Content.rn_header.rn_flags=3;
   n4->Content.rn_header.rn_flags=4;
   n5->Content.rn_header.rn_flags=5;

   struct NotificationNode* x = notificationQueueDequeueNotification(&nq, pr);
   int i=0;
   while(x) {
      printf(" #%d\n", x->Content.rn_header.rn_flags);
      free(x);
      x = notificationQueueDequeueNotification(&nq, pr);
      i++;
      if(i == 3) {
         struct NotificationNode* n6 = notificationQueueEnqueueNotification(&nq, pr, NT_SESSION_CHANGE);
         n6->Content.rn_header.rn_flags=6;
      }
   }


   notificationQueueDelete(&nq);
}
