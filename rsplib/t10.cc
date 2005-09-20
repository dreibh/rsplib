#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "thread.h"


class ThreadedServer : public TDThread
{
   public:
   ThreadedServer(int rserpoolSocketDescriptor);
   ~ThreadedServer();

   inline bool hasFinished() const {
      return(RSerPoolSocketDescriptor < 0);
   }

   protected:
   int  RSerPoolSocketDescriptor;

   protected:
   virtual void handleMessage(const char* buffer, size_t bufferSize, uint32_t ppid) = 0;
   virtual void handleCookieEcho(const char* buffer, size_t bufferSize);
   virtual void handleNotification(const union rserpool_notification* notification);

   private:
   void run();

   bool IsNewSession;
   bool Shutdown;
};


// ###### Constructor #######################################################
ThreadedServer::ThreadedServer(int rserpoolSocketDescriptor)
{
   RSerPoolSocketDescriptor = rserpoolSocketDescriptor;
   IsNewSession = true;
   Shutdown     = false;
   printf("New thread for RSerPool socket %d.\n", RSerPoolSocketDescriptor);
}


// ###### Destructor ########################################################
ThreadedServer::~ThreadedServer()
{
   Shutdown = true;
   waitForFinish();
   printf("Thread for RSerPool socket %d has been stopped.\n", RSerPoolSocketDescriptor);
   if(RSerPoolSocketDescriptor >= 0) {
      rsp_close(RSerPoolSocketDescriptor);
      RSerPoolSocketDescriptor = -1;
   }
}


// ###### Handle cookie #####################################################
void ThreadedServer::handleCookieEcho(const char* buffer, size_t bufferSize)
{
   puts("COOKIE ECHO");
}


// ###### Handle notification ###############################################
void ThreadedServer::handleNotification(const union rserpool_notification* notification)
{
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
}


// ###### Threaded server main loop #########################################
void ThreadedServer::run()
{
   char                       buffer[65536];
   struct rserpool_sndrcvinfo rinfo;
   ssize_t                    received;
   int                        flags;

   while(!Shutdown) {
      flags     = 0;
      received = rsp_recvmsg(RSerPoolSocketDescriptor,
                             (char*)&buffer, sizeof(buffer),
                             &rinfo, &flags, 1000000);
      if(received > 0) {
         if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
            handleCookieEcho((char*)&buffer, received);
         }
         else if(flags & MSG_RSERPOOL_NOTIFICATION) {
            if(received >= (ssize_t)sizeof(union rserpool_notification)) {
               handleNotification((union rserpool_notification*)&buffer);
            }
         }
         else {
            handleMessage((char*)&buffer, received, rinfo.rinfo_ppid);
         }
      }
      else if(received == 0) {
         puts("--- Abbruch ---");
         Shutdown = true;
         break;
      }
   }

   puts("Thread-Ende!\n");
}




class ThreadedServerList
{
   public:
   ThreadedServerList();
   ~ThreadedServerList();
   void add(ThreadedServer* thread);
   void remove(ThreadedServer* thread);
   void removeFinished();
   void removeAll();

   private:
   struct ThreadListEntry {
      ThreadListEntry* Next;
      ThreadedServer*  Object;
   };
   ThreadListEntry* ThreadList;
};


// ###### Constructor #######################################################
ThreadedServerList::ThreadedServerList()
{
   ThreadList = NULL;
}


// ###### Destructor ########################################################
ThreadedServerList::~ThreadedServerList()
{
   removeAll();
}


// ###### Remove finished sessions ##########################################
void ThreadedServerList::removeFinished()
{
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      ThreadListEntry* next = entry->Next;
      if(entry->Object->hasFinished()) {
         remove(entry->Object);
      }
      entry = next;
   }
}


// ###### Remove all sessions ###############################################
void ThreadedServerList::removeAll()
{
   ThreadListEntry* entry = ThreadList;
   while(entry != NULL) {
      remove(entry->Object);
      entry = ThreadList;
   }
}


// ###### Add session #######################################################
void ThreadedServerList::add(ThreadedServer* thread)
{
   ThreadListEntry* entry = new ThreadListEntry;
   entry->Next   = ThreadList;
   entry->Object = thread;
   ThreadList    = entry;
}


// ###### Remove session ####################################################
void ThreadedServerList::remove(ThreadedServer* thread)
{
   ThreadListEntry* entry = ThreadList;
   ThreadListEntry* prev  = NULL;
   while(entry != NULL) {
      if(entry->Object == thread) {
         if(prev == NULL) {
            ThreadList = entry->Next;
         }
         else {
            prev->Next = entry->Next;
         }
         delete entry->Object;
         entry->Object = NULL;
         delete entry;
         return;
      }
      prev  = entry;
      entry = entry->Next;
   }
}




class PingPongServer : public ThreadedServer
{
   public:
   PingPongServer(int rserpoolSocketDescriptor);
   ~PingPongServer();

   protected:
   void handleMessage(const char* buffer, size_t bufferSize, uint32_t ppid);

   private:
   uint32_t ReplyNo;
};


PingPongServer::PingPongServer(int rserpoolSocketDescriptor)
   : ThreadedServer(rserpoolSocketDescriptor)
{
   ReplyNo = 1;
}


PingPongServer::~PingPongServer()
{
}


void PingPongServer::handleMessage(const char* buffer, size_t bufferSize, uint32_t ppid)
{
   ssize_t sent;

   if(bufferSize >= (ssize_t)sizeof(PingPongCommonHeader)) {
      const Ping* ping = (const Ping*)buffer;
      if(ping->Header.Type == PPPT_PING) {
         if(ntohs(ping->Header.Type) >= (ssize_t)sizeof(struct Ping)) {
            Pong pong;
            pong.Header.Type   = PPPT_PONG;
            pong.Header.Flags  = 0x00;
            pong.Header.Length = htons(sizeof(pong));
            pong.MessageNo     = ping->MessageNo;
            pong.ReplyNo       = hton64(ReplyNo);
            sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                               (char*)&pong, sizeof(pong), 0,
                               0, NULL, 0, PPID_PPP, 0, ~0, 0, NULL);
            printf("snd=%d\n",sent);
            if(sent > 0) {
               ReplyNo++;
            }
/*???           ssize_t sc = rsp_send_cookie(sd, (unsigned char*)&pong, sizeof(pong),
                                       rinfo.rinfo_session, 0, NULL);
            printf("sc=%d\n",sc);*/
         }
      }
   }
}
