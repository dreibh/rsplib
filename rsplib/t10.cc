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
      return(Finished);
   }
   void shutdown();

   static void poolElement(const char*          programTitle,
                           const char*          poolHandle,
                           struct rsp_loadinfo* loadinfo,
                           ThreadedServer*      (*threadFactory)(int sd));

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
   bool Finished;
};



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
ThreadedServer::ThreadedServer(int rserpoolSocketDescriptor)
{
   RSerPoolSocketDescriptor = rserpoolSocketDescriptor;
   IsNewSession = true;
   Shutdown     = false;
   Finished     = false;
   printTimeStamp(stdout);
   printf("New thread for RSerPool socket %d.\n", RSerPoolSocketDescriptor);
}


// ###### Destructor ########################################################
ThreadedServer::~ThreadedServer()
{
   shutdown();
   if(RSerPoolSocketDescriptor >= 0) {
      rsp_close(RSerPoolSocketDescriptor);
      RSerPoolSocketDescriptor = -1;
   }
}


// ###### Shutdown thread ###################################################
void ThreadedServer::shutdown()
{
   if(!Finished) {
      Shutdown = true;
      waitForFinish();
      printTimeStamp(stdout);
      printf("Thread for RSerPool socket %d has been stopped.\n", RSerPoolSocketDescriptor);
   }
}


// ###### Handle cookie #####################################################
void ThreadedServer::handleCookieEcho(const char* buffer, size_t bufferSize)
{
   printTimeStamp(stdout);
   puts("COOKIE ECHO");
}


// ###### Handle notification ###############################################
void ThreadedServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
}


// ###### Threaded server main loop #########################################
void ThreadedServer::run()
{
   char                  buffer[65536];
   struct rsp_sndrcvinfo rinfo;
   ssize_t               received;
   int                   flags;

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
         break;
      }
   }

   Finished = true;
}


void ThreadedServer::poolElement(const char*          programTitle,
                                 const char*          poolHandle,
                                 struct rsp_loadinfo* loadinfo,
                                 ThreadedServer*      (*threadFactory)(int sd))
{
   beginLogging();
   if(rsp_initialize(NULL, 0) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      finishLogging();
      return;
   }

   int rserpoolSocket = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP, NULL);
   if(rserpoolSocket >= 0) {
      // ====== Initialize PE settings ======================================
      struct rsp_loadinfo dummyLoadinfo;
      if(loadinfo == NULL) {
         memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
         loadinfo = &dummyLoadinfo;
         loadinfo->rli_policy = PPT_ROUNDROBIN;
      }

      // ====== Print program title =========================================
      puts(programTitle);
      for(size_t i = 0;i < strlen(programTitle);i++) {
         printf("=");
      }
      puts("\n");

      // ====== Register PE =================================================
      if(rsp_register(rserpoolSocket,
                      (const unsigned char*)poolHandle, strlen(poolHandle),
                      loadinfo, NULL) < 0) {
         printf("Failed to register PE to pool %s\n", poolHandle);
      }

      // ====== Main loop ===================================================
      ThreadedServerList serverSet;
      installBreakDetector();
      while(!breakDetected()) {
         // ====== Clean-up session list ====================================
         serverSet.removeFinished();

         // ====== Wait for incoming sessions ===============================
         int newRSerPoolSocket = rsp_accept(rserpoolSocket, 500000, NULL);
         if(newRSerPoolSocket >= 0) {
            ThreadedServer* serviceThread = threadFactory(newRSerPoolSocket);
            if(serviceThread) {
               serverSet.add(serviceThread);
               serviceThread->start();
            }
            else {
               rsp_close(newRSerPoolSocket);
            }
         }
      }

      // ====== Clean up ====================================================
      serverSet.removeAll();
      rsp_deregister(rserpoolSocket);
      rsp_close(rserpoolSocket);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   finishLogging();
   puts("\nTerminated!");
}




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

         // ====== Tell thread to shut down =================================
         entry->Object->shutdown();

         // ====== Remove thread ============================================
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

   static ThreadedServer* pingPongServerFactory(int sd);

   protected:
   void handleMessage(const char* buffer, size_t bufferSize, uint32_t ppid);

   private:
   uint64_t ReplyNo;
};


PingPongServer::PingPongServer(int rserpoolSocketDescriptor)
   : ThreadedServer(rserpoolSocketDescriptor)
{
   ReplyNo = 1;
}


PingPongServer::~PingPongServer()
{
}


ThreadedServer* PingPongServer::pingPongServerFactory(int sd)
{
   return(new PingPongServer(sd));
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





class UDPLikeServer
{
   public:
   UDPLikeServer(int rserpoolSocketDescriptor);
   virtual ~UDPLikeServer();

   static void udpLikePoolElement(const char*          programTitle,
                                  const char*          poolHandle,
                                  struct rsp_loadinfo* loadinfo,
                                  UDPLikeServer*       server);

   protected:
   virtual void handleMessage(rserpool_session_t sessionID,
                              const char*        buffer,
                              size_t             bufferSize,
                              uint32_t           ppid) = 0;
   virtual void handleCookieEcho(rserpool_session_t sessionID,
                                 const char*        buffer,
                                 size_t             bufferSize);
   virtual void handleNotification(const union rserpool_notification* notification);

   protected:
   int RSerPoolSocketDescriptor;
};


// ###### Constructor #######################################################
UDPLikeServer::UDPLikeServer(int rserpoolSocketDescriptor)
{
   RSerPoolSocketDescriptor = rserpoolSocketDescriptor;
}


// ###### Destructor ########################################################
UDPLikeServer::~UDPLikeServer()
{
}


// ###### Handle cookie #####################################################
void UDPLikeServer::handleCookieEcho(rserpool_session_t sessionID,
                                     const char*        buffer,
                                     size_t             bufferSize)
{
   printTimeStamp(stdout);
   printf("COOKIE ECHO for session %u\n", sessionID);
}


// ###### Handle notification ###############################################
void UDPLikeServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
}


// ###### Implementation of a simple UDP-like server ########################
void UDPLikeServer::udpLikePoolElement(const char*          programTitle,
                                       const char*          poolHandle,
                                       struct rsp_loadinfo* loadinfo,
                                       UDPLikeServer*       server)
{
   beginLogging();
   if(rsp_initialize(NULL, 0) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      finishLogging();
      return;
   }

   int rserpoolSocket = rsp_socket(0, SOCK_STREAM, IPPROTO_SCTP, NULL);
   if(rserpoolSocket >= 0) {
      // ====== Initialize PE settings ======================================
      struct rsp_loadinfo dummyLoadinfo;
      if(loadinfo == NULL) {
         memset(&dummyLoadinfo, 0, sizeof(dummyLoadinfo));
         loadinfo = &dummyLoadinfo;
         loadinfo->rli_policy = PPT_ROUNDROBIN;
      }

      // ====== Print program title =========================================
      puts(programTitle);
      for(size_t i = 0;i < strlen(programTitle);i++) {
         printf("=");
      }
      puts("\n");

      // ====== Register PE =================================================
      if(rsp_register(rserpoolSocket,
                      (const unsigned char*)poolHandle, strlen(poolHandle),
                      loadinfo, NULL) < 0) {
         printf("Failed to register PE to pool %s\n", poolHandle);
      }

      // ====== Main loop ===================================================
      installBreakDetector();
      while(!breakDetected()) {
         // ====== Read from socket =========================================
         char                  buffer[65536];
         int                   flags = 0;
         struct rsp_sndrcvinfo rinfo;
         ssize_t received = rsp_recvmsg(rserpoolSocket, (char*)&buffer, sizeof(buffer),
                                        &rinfo, &flags, 500000);

         // ====== Handle data ==============================================
         if(received > 0) {
            if(flags & MSG_RSERPOOL_NOTIFICATION) {
               server->handleNotification((union rserpool_notification*)&buffer);
            }
            else if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
               server->handleCookieEcho(rinfo.rinfo_session,
                                        (char*)&buffer, received);
            }
            else {
               server->handleMessage(rinfo.rinfo_session,
                                     (char*)&buffer, received,
                                     rinfo.rinfo_ppid);
            }
         }
      }

      // ====== Clean up ====================================================
      rsp_deregister(rserpoolSocket);
      rsp_close(rserpoolSocket);
   }
   else {
      logerror("Unable to create RSerPool socket");
   }

   rsp_cleanup();
   finishLogging();
   puts("\nTerminated!");
}






class EchoServer : public UDPLikeServer
{
   public:
   EchoServer(int rserpoolSocketDescriptor);
   virtual ~EchoServer();

   protected:
   virtual void handleMessage(rserpool_session_t sessionID,
                              const char*        buffer,
                              size_t             bufferSize,
                              uint32_t           ppid);
};


// ###### Constructor #######################################################
EchoServer::EchoServer(int rserpoolSocketDescriptor)
   : UDPLikeServer(rserpoolSocketDescriptor)
{
}


// ###### Destructor ########################################################
EchoServer::~EchoServer()
{
}


// ###### Handle message ####################################################
void EchoServer::handleMessage(rserpool_session_t sessionID,
                               const char*        buffer,
                               size_t             bufferSize,
                               uint32_t           ppid)
{
   ssize_t sent;

   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                      buffer, bufferSize, 0,
                      0, NULL, 0, ppid, 0, ~0, 0, NULL);
   printf("snd=%d\n", sent);
}
