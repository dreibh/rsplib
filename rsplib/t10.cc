#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "thread.h"


enum EventHandlingResult
{
   EHR_Okay     = 0,
   EHR_Shutdown = 1,
   EHR_Abort    = 2
};


class ThreadedServer : public TDThread
{
   public:
   ThreadedServer(int rserpoolSocketDescriptor);
   ~ThreadedServer();

   inline bool hasFinished() const {
      return(Finished);
   }
   inline bool isShuttingDown() const {
      return(Shutdown);
   }
   void shutdown();

   static void poolElement(const char*          programTitle,
                           const char*          poolHandle,
                           struct rsp_loadinfo* loadinfo,
                           void*                userData,
                           ThreadedServer*      (*threadFactory)(int sd, void* userData));

   protected:
   int RSerPoolSocketDescriptor;

   protected:
   virtual EventHandlingResult handleMessage(const char* buffer,
                                             size_t      bufferSize,
                                             uint32_t    ppid,
                                             uint16_t    streamID) = 0;
   virtual EventHandlingResult handleCookieEcho(const char* buffer, size_t bufferSize);
   virtual EventHandlingResult handleNotification(const union rserpool_notification* notification);

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
   printTimeStamp(stdout);
   printf("Thread for RSerPool socket %d has been stopped.\n", RSerPoolSocketDescriptor);
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
   }
}


// ###### Handle cookie #####################################################
EventHandlingResult ThreadedServer::handleCookieEcho(const char* buffer, size_t bufferSize)
{
   printTimeStamp(stdout);
   puts("COOKIE ECHO");
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
EventHandlingResult ThreadedServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
   return(EHR_Okay);
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
         // ====== Handle event =============================================
         EventHandlingResult eventHandlingResult;
         if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
            if(IsNewSession) {
               eventHandlingResult = handleCookieEcho((char*)&buffer, received);
               IsNewSession = false;
            }
            else {
               printTimeStamp(stdout);
               puts("Dropped unexpected ASAP COOKIE_ECHO!");
               eventHandlingResult = EHR_Abort;
            }
         }
         else if(flags & MSG_RSERPOOL_NOTIFICATION) {
            if(received >= (ssize_t)sizeof(union rserpool_notification)) {
               eventHandlingResult = handleNotification((const union rserpool_notification*)&buffer);
            }
         }
         else {
            IsNewSession = false;
            eventHandlingResult = handleMessage((char*)&buffer, received,
                                                rinfo.rinfo_ppid, rinfo.rinfo_stream);
         }

         // ====== Check for problems =======================================
         if((eventHandlingResult == EHR_Abort) ||
            (eventHandlingResult == EHR_Shutdown)) {
            rsp_sendmsg(RSerPoolSocketDescriptor,
                        NULL, 0, (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_SHUTDOWN,
                        0, 0, 0, ~0, 0, NULL);
         }
      }
      else if(received == 0) {
         break;
      }
   }

   Finished = true;
}


// ###### Implementation of a simple threaded server ########################
void ThreadedServer::poolElement(const char*          programTitle,
                                 const char*          poolHandle,
                                 struct rsp_loadinfo* loadinfo,
                                 void*                userData,
                                 ThreadedServer*      (*threadFactory)(int sd, void* userData))
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
                      loadinfo, NULL) == 0) {

         // ====== Main loop ===================================================
         ThreadedServerList serverSet;
         installBreakDetector();
         while(!breakDetected()) {
            // ====== Clean-up session list ====================================
            serverSet.removeFinished();

            // ====== Wait for incoming sessions ===============================
            int newRSerPoolSocket = rsp_accept(rserpoolSocket, 500000, NULL);
            if(newRSerPoolSocket >= 0) {
               ThreadedServer* serviceThread = threadFactory(newRSerPoolSocket, userData);
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
      }
      else {
         printf("Failed to register PE to pool %s\n", poolHandle);
      }
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
         entry->Object->waitForFinish();

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

   static ThreadedServer* pingPongServerFactory(int sd, void* userData);

   protected:
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   uint64_t ReplyNo;
};


// ###### Constructor #######################################################
PingPongServer::PingPongServer(int rserpoolSocketDescriptor)
   : ThreadedServer(rserpoolSocketDescriptor)
{
   ReplyNo = 1;
}


// ###### Destructor ########################################################
PingPongServer::~PingPongServer()
{
}


// ###### Create a PingServer thread ########################################
ThreadedServer* PingPongServer::pingPongServerFactory(int sd, void* userData)
{
   return(new PingPongServer(sd));
}


// ###### Handle message ####################################################
EventHandlingResult PingPongServer::handleMessage(const char* buffer,
                                                  size_t      bufferSize,
                                                  uint32_t    ppid,
                                                  uint16_t    streamID)
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
                               0, PPID_PPP, 0, ~0, 0, NULL);
            printf("snd=%d\n",sent);
            if(sent > 0) {
               ReplyNo++;
               return(EHR_Okay);
            }
         }
      }
   }
   return(EHR_Abort);
}





class UDPLikeServer
{
   public:
   UDPLikeServer();
   virtual ~UDPLikeServer();

   virtual void poolElement(const char*          programTitle,
                            const char*          poolHandle,
                            struct rsp_loadinfo* loadinfo);

   protected:
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID) = 0;
   virtual EventHandlingResult handleCookieEcho(rserpool_session_t sessionID,
                                                const char*        buffer,
                                                size_t             bufferSize);
   virtual EventHandlingResult handleNotification(const union rserpool_notification* notification);

   protected:
   int RSerPoolSocketDescriptor;
};


// ###### Constructor #######################################################
UDPLikeServer::UDPLikeServer()
{
   RSerPoolSocketDescriptor = -1;
}


// ###### Destructor ########################################################
UDPLikeServer::~UDPLikeServer()
{
}


// ###### Handle cookie #####################################################
EventHandlingResult UDPLikeServer::handleCookieEcho(rserpool_session_t sessionID,
                                     const char*        buffer,
                                     size_t             bufferSize)
{
   printTimeStamp(stdout);
   printf("COOKIE ECHO for session %u\n", sessionID);
   return(EHR_Okay);
}


// ###### Handle notification ###############################################
EventHandlingResult UDPLikeServer::handleNotification(const union rserpool_notification* notification)
{
   printTimeStamp(stdout);
   printf("NOTIFICATION: ");
   rsp_print_notification(notification, stdout);
   puts("");
   return(EHR_Okay);
}


// ###### Implementation of a simple UDP-like server ########################
void UDPLikeServer::poolElement(const char*          programTitle,
                                const char*          poolHandle,
                                struct rsp_loadinfo* loadinfo)
{
   beginLogging();
   if(rsp_initialize(NULL, 0) < 0) {
      puts("ERROR: Unable to initialize rsplib Library!\n");
      finishLogging();
      return;
   }

   RSerPoolSocketDescriptor = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP, NULL);
   if(RSerPoolSocketDescriptor >= 0) {
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
      if(rsp_register(RSerPoolSocketDescriptor,
                      (const unsigned char*)poolHandle, strlen(poolHandle),
                      loadinfo, NULL) == 0) {

         // ====== Main loop ================================================
         installBreakDetector();
         while(!breakDetected()) {
            // ====== Read from socket ======================================
            char                  buffer[65536];
            int                   flags = 0;
            struct rsp_sndrcvinfo rinfo;
            ssize_t received = rsp_recvmsg(RSerPoolSocketDescriptor, (char*)&buffer, sizeof(buffer),
                                          &rinfo, &flags, 500000);

            // ====== Handle data ===========================================
            if(received > 0) {
               // ====== Handle event =======================================
               EventHandlingResult eventHandlingResult;
               if(flags & MSG_RSERPOOL_NOTIFICATION) {
                  handleNotification((const union rserpool_notification*)&buffer);
                  /*
                     We cannot shutdown or abort, since the session ID is not
                     inserted into rinfo!
                     Should we dissect the notification for the ID here?
                  */
                  eventHandlingResult = EHR_Okay;
               }
               else if(flags & MSG_RSERPOOL_COOKIE_ECHO) {
                  eventHandlingResult = handleCookieEcho(rinfo.rinfo_session,
                                                         (char*)&buffer, received);
               }
               else {
                  eventHandlingResult = handleMessage(rinfo.rinfo_session,
                                                      (char*)&buffer, received,
                                                      rinfo.rinfo_ppid,
                                                      rinfo.rinfo_stream);
               }

               // ====== Check for problems =================================
               if((eventHandlingResult == EHR_Abort) ||
                  (eventHandlingResult == EHR_Shutdown)) {
                  rsp_sendmsg(RSerPoolSocketDescriptor,
                              NULL, 0, (eventHandlingResult == EHR_Abort) ? SCTP_ABORT : SCTP_SHUTDOWN,
                              rinfo.rinfo_session, 0, 0, ~0, 0, NULL);
               }
            }
         }

         // ====== Clean up =================================================
         rsp_deregister(RSerPoolSocketDescriptor);
      }
      else {
         printf("Failed to register PE to pool %s\n", poolHandle);
      }
      rsp_close(RSerPoolSocketDescriptor);
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
   EchoServer();
   virtual ~EchoServer();

   protected:
   virtual EventHandlingResult handleMessage(rserpool_session_t sessionID,
                                             const char*        buffer,
                                             size_t             bufferSize,
                                             uint32_t           ppid,
                                             uint16_t           streamID);
};


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
                      ~0, 0, NULL);
   printf("snd=%d\n", sent);
   return((sent == (ssize_t)bufferSize) ? EHR_Okay : EHR_Abort);
}








#include <complex>

#include "fractalgeneratorpackets.h"


using namespace std;


class FractalGeneratorServer : public ThreadedServer
{
   public:
   struct FractalGeneratorServerSettings
   {
      size_t FailureAfter;
      bool  TestMode;
   };

   FractalGeneratorServer(int                             rserpoolSocketDescriptor,
                          FractalGeneratorServerSettings* settings);
   ~FractalGeneratorServer();

   static ThreadedServer* fractalGeneratorServerFactory(int sd, void* userData);

   protected:
   EventHandlingResult handleCookieEcho(const char* buffer, size_t bufferSize);
   EventHandlingResult handleMessage(const char* buffer,
                                     size_t      bufferSize,
                                     uint32_t    ppid,
                                     uint16_t    streamID);

   private:
   bool sendCookie();
   bool sendData(FGPData* data);
   bool handleParameter(const FGPParameter* parameter,
                        const size_t        size);
   EventHandlingResult calculateImage();

   FractalGeneratorStatus         Status;
   unsigned long long             LastCookieTimeStamp;
   unsigned long long             LastSendTimeStamp;
   FractalGeneratorServerSettings Settings;
};


// ###### Constructor #######################################################
FractalGeneratorServer::FractalGeneratorServer(int rserpoolSocketDescriptor,
                                               FractalGeneratorServer::FractalGeneratorServerSettings* settings)
   : ThreadedServer(rserpoolSocketDescriptor)
{
   Settings            = *settings;
   LastSendTimeStamp   = 0;
   LastCookieTimeStamp = 0;
}


// ###### Destructor ########################################################
FractalGeneratorServer::~FractalGeneratorServer()
{
}


// ###### Create a FractalGenerator thread ##################################
ThreadedServer* FractalGeneratorServer::fractalGeneratorServerFactory(int sd, void* userData)
{
   return(new FractalGeneratorServer(sd, (FractalGeneratorServerSettings*)userData));
}


// ###### Send cookie #######################################################
bool FractalGeneratorServer::sendCookie()
{
   FGPCookie cookie;
   ssize_t   sent;

   strncpy((char*)&cookie.ID, FGP_COOKIE_ID, sizeof(cookie.ID));
   cookie.Parameter.Header.Type   = FGPT_PARAMETER;
   cookie.Parameter.Header.Flags  = 0x00;
   cookie.Parameter.Header.Length = htons(sizeof(cookie.Parameter.Header));
   cookie.Parameter.Width         = htonl(Status.Parameter.Width);
   cookie.Parameter.Height        = htonl(Status.Parameter.Height);
   cookie.Parameter.MaxIterations = htonl(Status.Parameter.MaxIterations);
   cookie.Parameter.AlgorithmID   = htonl(Status.Parameter.AlgorithmID);
   cookie.Parameter.C1Real        = Status.Parameter.C1Real;
   cookie.Parameter.C2Real        = Status.Parameter.C2Real;
   cookie.Parameter.C1Imag        = Status.Parameter.C1Imag;
   cookie.Parameter.C2Imag        = Status.Parameter.C2Imag;
   cookie.Parameter.N             = Status.Parameter.N;
   cookie.CurrentX                = htonl(Status.CurrentX);
   cookie.CurrentY                = htonl(Status.CurrentY);

   sent = rsp_send_cookie(RSerPoolSocketDescriptor,
                          (unsigned char*)&cookie, sizeof(cookie),
                          0, 0, NULL);

   LastCookieTimeStamp = getMicroTime();
   return(sent == sizeof(cookie));
}


// ###### Send data #########################################################
bool FractalGeneratorServer::sendData(FGPData* data)
{
   const size_t dataSize = getFGPDataSize(data->Points);
   ssize_t      sent;

   for(size_t i = 0;i < data->Points;i++) {
      data->Buffer[i] = htonl(data->Buffer[i]);
   }
   data->Header.Type   = FGPT_DATA;
   data->Header.Flags  = 0x00;
   data->Header.Length = htons(dataSize);
   data->Points = htonl(data->Points);
   data->StartX = htonl(data->StartX);
   data->StartY = htonl(data->StartY);

   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                        data, dataSize, 0,
                        0, PPID_FGP, 0, ~0, 0, NULL);

   data->Points = 0;
   data->StartX = 0;
   data->StartY = 0;
   LastSendTimeStamp = getMicroTime();

   return(sent == (ssize_t)dataSize);
}


// ###### Handle parameter message ##########################################
bool FractalGeneratorServer::handleParameter(const FGPParameter* parameter,
                                             const size_t        size)
{
   if(size < sizeof(struct FGPParameter)) {
      printTimeStamp(stdout);
      printf("Received too short message on RSerPool socket %u\n",
             RSerPoolSocketDescriptor);
      return(false);
   }
   if(parameter->Header.Type != FGPT_PARAMETER) {
      printTimeStamp(stdout);
      printf("Received unknown message type %u on RSerPool socket %u\n",
             parameter->Header.Type,
             RSerPoolSocketDescriptor);
      return(false);
   }
   Status.Parameter.Width         = ntohl(parameter->Width);
   Status.Parameter.Height        = ntohl(parameter->Height);
   Status.Parameter.C1Real        = parameter->C1Real;
   Status.Parameter.C2Real        = parameter->C2Real;
   Status.Parameter.C1Imag        = parameter->C1Imag;
   Status.Parameter.C2Imag        = parameter->C2Imag;
   Status.Parameter.N             = parameter->N;
   Status.Parameter.MaxIterations = ntohl(parameter->MaxIterations);
   Status.Parameter.AlgorithmID   = ntohl(parameter->AlgorithmID);
   Status.CurrentX                = 0;
   Status.CurrentY                = 0;

   printTimeStamp(stdout);
   printf("Got Parameter on RSerPool socket %u:\nw=%u h=%u c1=(%lf,%lf) c2=(%lf,%lf), n=%f, maxIterations=%u, algorithmID=%u\n",
          RSerPoolSocketDescriptor,
          Status.Parameter.Width,
          Status.Parameter.Height,
          networkToDouble(Status.Parameter.C1Real),
          networkToDouble(Status.Parameter.C1Imag),
          networkToDouble(Status.Parameter.C2Real),
          networkToDouble(Status.Parameter.C2Imag),
          networkToDouble(Status.Parameter.N),
          Status.Parameter.MaxIterations,
          Status.Parameter.AlgorithmID);
   if((Status.Parameter.Width > 65536)  ||
      (Status.Parameter.Height > 65536) ||
      (Status.Parameter.MaxIterations > 1000000)) {
      puts("Bad parameters!");
      return(false);
   }
   return(true);
}


// ###### Handle cookie echo ################################################
EventHandlingResult FractalGeneratorServer::handleCookieEcho(const char* buffer,
                                                             size_t      bufferSize)
{
   const struct FGPCookie* cookie = (const struct FGPCookie*)buffer;

   if(bufferSize != sizeof(struct FGPCookie)) {
      printTimeStamp(stdout);
      printf("Invalid size of cookie on RSerPool socket %u\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   if(strncmp(cookie->ID, FGP_COOKIE_ID, sizeof(cookie->ID))) {
      printTimeStamp(stdout);
      printf("Invalid cookie ID on RSerPool socket %u\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   if(!handleParameter(&cookie->Parameter, sizeof(cookie->Parameter))) {
      return(EHR_Abort);
   }

   Status.CurrentX = ntohl(cookie->CurrentX);
   Status.CurrentY = ntohl(cookie->CurrentY);
   printTimeStamp(stdout);
   printf("Got CookieEcho on RSerPool socket %u:\nx=%u y=%u\n",
          RSerPoolSocketDescriptor,
          Status.CurrentX, Status.CurrentY);

   return(calculateImage());
}


// ###### Handle message ####################################################
EventHandlingResult FractalGeneratorServer::handleMessage(const char* buffer,
                                                          size_t      bufferSize,
                                                          uint32_t    ppid,
                                                          uint16_t    streamID)
{
   if(!handleParameter((const FGPParameter*)buffer, bufferSize)) {
      return(EHR_Abort);
   }
   return(calculateImage());
}


// ###### Calculate image ####################################################
EventHandlingResult FractalGeneratorServer::calculateImage()
{
   // ====== Initialize =====================================================
   struct FGPData  data;
   double          stepX;
   double          stepY;
   complex<double> z;
   size_t          i;
   size_t          dataPackets = 0;

   data.Points = 0;
   const double c1real = networkToDouble(Status.Parameter.C1Real);
   const double c1imag = networkToDouble(Status.Parameter.C1Imag);
   const double c2real = networkToDouble(Status.Parameter.C2Real);
   const double c2imag = networkToDouble(Status.Parameter.C2Imag);
   const double n      = networkToDouble(Status.Parameter.N);
   stepX = (c2real - c1real) / Status.Parameter.Width;
   stepY = (c2imag - c1imag) / Status.Parameter.Height;
   LastCookieTimeStamp = LastSendTimeStamp = getMicroTime();

   if(!sendCookie()) {
      printTimeStamp(stdout);
      printf("Sending cookie (start) on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }

   // ====== Calculate image ================================================
   while(Status.CurrentY < Status.Parameter.Height) {
      while(Status.CurrentX < Status.Parameter.Width) {
         // ====== Calculate pixel ==========================================
         if(!Settings.TestMode) {
            const complex<double> c =
               complex<double>(c1real + ((double)Status.CurrentX * stepX),
                               c1imag + ((double)Status.CurrentY * stepY));

            // ====== Algorithms ============================================
            switch(Status.Parameter.AlgorithmID) {
               case FGPA_MANDELBROT:
                  z = complex<double>(0.0, 0.0);
                  for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                     z = z*z - c;
                     if(z.real() * z.real() + z.imag() * z.imag() >= 2.0) {
                        break;
                     }
                  }
                break;
               case FGPA_MANDELBROTN:
                  z = complex<double>(0.0, 0.0);
                  for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                     z = pow(z, (int)rint(n)) - c;
                     if(z.real() * z.real() + z.imag() * z.imag() >= 2.0) {
                        break;
                     }
                  }
                break;
               default:
                  i = 0;
                break;
            }
         }
         else {
            i = (Status.CurrentX * Status.CurrentY) % 256;
         }
         // =================================================================

         if(data.Points == 0) {
            data.StartX = Status.CurrentX;
            data.StartY = Status.CurrentY;
         }
         data.Buffer[data.Points] = i;
         data.Points++;

         // ====== Send data ================================================
         if( (data.Points >= FGD_MAX_POINTS) ||
            (LastSendTimeStamp + 1000000 < getMicroTime()) ) {
            if(!sendData(&data)) {
               printTimeStamp(stdout);
               printf("Sending data on RSerPool socket %u failed!\n",
                      RSerPoolSocketDescriptor);
               return(EHR_Abort);
            }

            dataPackets++;
            if((Settings.FailureAfter > 0) && (dataPackets >= Settings.FailureAfter)) {
               sendCookie();
               printTimeStamp(stdout);
               printf("Failure Tester on RSerPool socket %u -> Disconnecting after %u packets!\n",
                      RSerPoolSocketDescriptor,
                      dataPackets);
               return(EHR_Shutdown);
            }
         }

         Status.CurrentX++;
      }
      Status.CurrentX = 0;
      Status.CurrentY++;

      // ====== Send cookie =================================================
      if(LastCookieTimeStamp + 1000000 < getMicroTime()) {
         if(!sendCookie()) {
            printTimeStamp(stdout);
            printf("Sending cookie (start) on RSerPool socket %u failed!\n",
                  RSerPoolSocketDescriptor);
            return(EHR_Abort);
         }
      }
      if(isShuttingDown()) {
         printf("Aborting session on RSerPool socket %u due to server shutdown!\n",
               RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }
   }

   if(data.Points > 0) {
      if(!sendData(&data)) {
         printf("Sending data (last block) on RSerPool socket %u failed!\n",
                  RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }
   }

   data.StartX = 0xffffffff;
   data.StartY = 0xffffffff;
   if(!sendData(&data)) {
      printf("Sending data (end of data) on RSerPool socket %u failed!\n",
               RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }

   return(EHR_Okay);
}
