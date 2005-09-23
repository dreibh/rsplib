/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id: cspmonitor.c 0 2005-03-02 13:34:16Z dreibh $
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

#include "tdtypes.h"
#include "loglevel.h"
#include "netutilities.h"
#include "breakdetector.h"
#include "rsplib.h"
#include "calcapppackets.h"
#include "stdlib.h"
#include "randomizer.h"
#include "statistics.h"

#include <ext_socket.h>
#include <signal.h>


#include <iostream>


using namespace std;


/* Exit immediately on Ctrl-C. No clean shutdown. */
/* #define FAST_BREAK */

struct Job
{
   Job*               Next;

   unsigned int       JobID;
   double             JobSize;

   unsigned long long CompleteTimeStamp;
   unsigned long long QueuingTimeStamp;
   unsigned long long StartupTimeStamp;
   unsigned long long AcceptTimeStamp;
};


class JobQueue
{
   public:
   JobQueue();
   ~JobQueue();

   void enqueue(Job* job);
   Job* dequeue();

   private:
   Job* FirstJob;
   Job* LastJob;

   public:
   void PrintStatistics()
   {
        /*   Job* job = FirstJob;
         while(job != NULL)
         {

         cout << job->QueuingTimeStamp << endl;

         job = job->Next;
         }
   */
   return;
   }

};
JobQueue :: JobQueue()
{
   FirstJob = NULL;
   LastJob  = NULL;
}
JobQueue :: ~JobQueue()
{
   FirstJob = NULL;
   LastJob  = NULL;


}




void JobQueue::enqueue(Job* job)
{
   job->Next = NULL;
      job->QueuingTimeStamp = getMicroTime();
      job->StartupTimeStamp = 0ULL;


   if (FirstJob == NULL && LastJob == NULL)
   {
      FirstJob = job;
      LastJob  = job;
   }
   else
   {
      LastJob ->Next = job;
      LastJob = job;
   }
}

Job* JobQueue::dequeue()
{
   Job* job;
   job = FirstJob;
   if (FirstJob == LastJob)
   {
      FirstJob = NULL;
      LastJob  = NULL;
   }
   else
   {
      FirstJob = FirstJob -> Next;
   }

   if (job != NULL)
   {
     job->StartupTimeStamp = getMicroTime();
   }

   return(job);
}

unsigned long long KeepAliveTransmissionInterval = 5000000;
unsigned long long KeepAliveTimeoutInterval      = 5000000;
unsigned long long JobInterval                   = 15000000;
FILE*        VectorFH   = NULL;
FILE*        ScalarFH   = NULL;
unsigned int VectorLine = 0;
double       JobSizeDeclaration			 = 5000000;
unsigned long long runtime;
double TotalHandlingDelay            = 0.0;
double AverageHandlingDelay          = 0.0;
double TotalHandlingSpeed            = 0.0;
double AverageHandlingSpeed          = 0.0;
double AverageJobSize    	     = 0.0;
unsigned long long TotalJobInterval  = 0;
unsigned long long AverageJobInterval= 0;
unsigned int TotalJobsQueued         = 0;
unsigned int TotalJobsStarted        = 0;
double TotalJobSizeQueued            = 0.0;
double TotalJobSizeStarted           = 0.0;
double TotalJobSizeCompleted         = 0.0;
Statistics stat1;
Statistics stat2;
Statistics stat3;
Statistics stat4;
Statistics stat5;
Statistics stat6;
Statistics stat7;
Statistics stat8;
Statistics stat9;
Statistics stat10;
Statistics stat11;
Statistics stat12;

enum ProcessStatus {
   PS_Init       = 0,
   PS_Processing = 1,
   PS_Failover   = 2,
   PS_Failed     = 3,
   PS_Completed  = 4
};

struct Process {
   SessionDescriptor* Session;
   ProcessStatus      Status;
   JobQueue           Queue;
   Job*               CurrentJob;

   size_t             TotalCalcAppRequests;
   size_t	      TotalCalcAppAccepted;
   size_t             TotalCalcAppRejected;
   size_t             TotalCalcAppKeepAlives;
   size_t             TotalCalcAppKeepAliveAcks;
   size_t             TotalCalcAppCompleted;

   // ------ Timers ------------------------------------------------------
   unsigned long long NextJobTimeStamp;
   unsigned long long KeepAliveTransmissionTimeStamp;
   unsigned long long KeepAliveTimeoutTimeStamp;
};


// ###### Send CalcAppRequest message #######################################
void sendCalcAppRequest(struct Process* process)
{
   CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type    = htonl(CALCAPP_REQUEST);
   message.JobID   = process->CurrentJob->JobID;
   message.JobSize = process->CurrentJob->JobSize;

   ssize_t result = rspSessionWrite(process->Session,
                                    (void*)&message, sizeof(message), NULL);
   if(result <= 0) {
      process->Status = PS_Failed;
   }

   process->TotalCalcAppRequests++;
}


// ###### Send CalcAppKeepAlive message #####################################
void sendCalcAppKeepAlive(struct Process* process)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_KEEPALIVE);
   message.JobID = htonl(process->CurrentJob->JobID);

   ssize_t result = rspSessionWrite(process->Session,
                                    (void*)&message, sizeof(message), NULL);
   if(result <= 0) {
      cerr << "WARNING: Unable to send CalcAppKeepAlive" << endl;
      process->Status = PS_Failover;
   }
   process->TotalCalcAppKeepAlives++;
}


// ###### Send CalcAppKeepAliveAck message ##################################
void sendCalcAppKeepAliveAck(struct Process* process)
{
   struct CalcAppMessage message;
   memset(&message, 0, sizeof(message));
   message.Type  = htonl(CALCAPP_KEEPALIVE_ACK);
   message.JobID = htonl(process->CurrentJob->JobID);

   ssize_t result = rspSessionWrite(process->Session,
                                    (void*)&message, sizeof(message), NULL);
   if(result <= 0) {
      cerr << "WARNING: Unable to send CalcAppKeepAliveAck" << endl;
      process->Status = PS_Failover;
   }
   process->TotalCalcAppKeepAliveAcks++;
}


// ###### Handle incoming CalcAppAccept message #############################
void handleCalcAppAccept(struct Process* process,
                         CalcAppMessage* message,
                         const size_t    size)
{
   if(process->CurrentJob->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppAccept for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   cout << "Job " << process->CurrentJob->JobID << " accepted" << endl;
   process->CurrentJob->AcceptTimeStamp = getMicroTime();

   process->Status                         = PS_Processing;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;

   process->TotalCalcAppAccepted++;
}


// ###### Handle incoming CalcAppReject message #############################
void handleCalcAppReject(struct Process* process,
                         CalcAppMessage* message,
                         const size_t    size)
{
   if(process->CurrentJob->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppReject for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   cout << "Job " << process->CurrentJob->JobID << " rejected" << endl;
   process->Status                         = PS_Failover;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = ~0ULL;
   rspSessionFailover(process->Session);

   if(!rspSessionHasCookie(process->Session)) {
      process->Status = PS_Init;
      sendCalcAppRequest(process);
   }

   process->TotalCalcAppRejected++;
}


// ###### Handle incoming CalcAppKeepAlive message ##########################
void handleCalcAppKeepAlive(struct Process* process,
                            CalcAppMessage* message,
                            const size_t    size)
{
   if(process->CurrentJob->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAlive for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }
   sendCalcAppKeepAliveAck(process);
}


// ###### Handle incoming CalcAppKeepAliveAck message #######################
void handleCalcAppKeepAliveAck(struct Process* process,
                               CalcAppMessage* message,
                               const size_t    size)
{
   if(process->CurrentJob->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppKeepAliveAck for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   process->Status                         = PS_Processing;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;
}


// ###### Handle incoming CalcAppCompleted message ##########################
void handleCalcAppCompleted(struct Process* process,
                            CalcAppMessage* message,
                            const size_t    size)
{
   double HandlingDelay = 0.0;
   double HandlingSpeed = 0.0;
   double QueueingTime = 0.0;
   double StartupTime = 0.0;
   double ProcessingTime = 0.0;


   if(process->CurrentJob->JobID != ntohl(message->JobID)) {
      cerr << "ERROR: CalcAppCompleted for wrong job!" << endl;
      process->Status = PS_Failed;
      return;
   }

   cout << "Job " << process->CurrentJob->JobID << " completed" << endl;
   process->Status                         = PS_Completed;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = ~0ULL;
   process->CurrentJob->CompleteTimeStamp  = getMicroTime();

   QueueingTime = process->CurrentJob->StartupTimeStamp - process->CurrentJob->QueuingTimeStamp;
   StartupTime = process->CurrentJob->AcceptTimeStamp - process->CurrentJob->StartupTimeStamp;
   ProcessingTime = process->CurrentJob->CompleteTimeStamp - process->CurrentJob->AcceptTimeStamp;

   HandlingDelay = (QueueingTime + StartupTime + ProcessingTime)/ 1000000;
   HandlingSpeed = process->CurrentJob->JobSize / HandlingDelay ;
   TotalHandlingDelay+= HandlingDelay;
   TotalJobSizeCompleted+= process->CurrentJob->JobSize;
   TotalJobInterval+= JobInterval;
   TotalHandlingSpeed+=HandlingSpeed;

   stat1.collect(StartupTime/1000000.0);
   stat2.collect(QueueingTime/1000000.0);
   stat3.collect(ProcessingTime/1000000.0);
   stat4.collect(HandlingDelay);
   stat5.collect(HandlingSpeed/1000000.0);
   stat6.collect(process->CurrentJob->JobSize);
   stat7.collect(JobInterval/1000000.0);
   stat8.collect(StartupTime/1000000.0);
   stat9.collect(QueueingTime/1000000.0);
   stat10.collect(ProcessingTime/1000000.0);
   stat11.collect(HandlingDelay);
   stat12.collect(HandlingSpeed/1000000.0);

   cout << "JobSize:     " << process->CurrentJob->JobSize << endl;
   cout << "StartupTime: " << StartupTime << " QueueingTime: " << QueueingTime << " Processing Time: " << ProcessingTime << endl;
   cout << "Handling Delay: " << HandlingDelay << " Handling Speed: " << HandlingSpeed << endl;

   fprintf(VectorFH," %u %u %1.0f %llu %1.6f %1.6f %1.6f %1.6f %1.0f\n", ++VectorLine, process->CurrentJob->JobID, process->CurrentJob->JobSize, JobInterval, QueueingTime, StartupTime, ProcessingTime, HandlingDelay, HandlingSpeed);
   process->TotalCalcAppCompleted++;

}


// ###### Handle next job timer #############################################
void handleNextJobTimer(struct Process* process)
{
   static unsigned int jobID = 0;

   Job* job = new Job;
   CHECK(job != NULL);
   job->JobID   = ++jobID;
   job->JobSize = randomExpDouble(JobSizeDeclaration);

   process->Queue.enqueue(job);
   TotalJobsQueued++;
   TotalJobSizeQueued+=job->JobSize;
   process->NextJobTimeStamp = getMicroTime() + (unsigned long long)randomExpDouble(JobInterval);
}


// ###### Handle KeepAlive Transmission timer ###############################
void handleKeepAliveTransmissionTimer(struct Process* process)
{
   process->KeepAliveTransmissionTimeStamp = ~0ULL;
   process->KeepAliveTimeoutTimeStamp      = getMicroTime() + KeepAliveTimeoutInterval;
   sendCalcAppKeepAlive(process);
}


// ###### Handle KeepAlive Timeout timer ####################################
bool handleKeepAliveTimeoutTimer(struct Process* process)
{
   cout << "Timeout -> Failover" << endl;

   rspSessionFailover(process->Session);

   process->Status                         = PS_Processing;
   process->KeepAliveTimeoutTimeStamp      = ~0ULL;
   process->KeepAliveTransmissionTimeStamp = getMicroTime() + KeepAliveTransmissionInterval;

   return(true);
}


// ###### Handle failover ###################################################
void handleFailover(Process* process)
{
   puts("===== FAILOVER ======");
   process->Status = PS_Processing;
}


// ###### Handle session events #############################################
void handleEvents(Process*           process,
                  const unsigned int sessionEvents)
{
   struct TagItem tags[16];
   char           buffer[4096];
   ssize_t        received;

   tags[0].Tag  = TAG_RspIO_Timeout;
   tags[0].Data = 0;
   tags[1].Tag  = TAG_DONE;
   received = rspSessionRead(process->Session, (char*)&buffer, sizeof(buffer),
                             (struct TagItem*)&tags);
   if(received > 0) {

      if(received >= (ssize_t)sizeof(CalcAppMessage)) {
         CalcAppMessage* response = (CalcAppMessage*)&buffer;
         switch(ntohl(response->Type)) {
            case CALCAPP_ACCEPT:
               handleCalcAppAccept(process, response, sizeof(response));
             break;
            case CALCAPP_REJECT:
               handleCalcAppReject(process, response, sizeof(response));
             break;
            case CALCAPP_KEEPALIVE:
               handleCalcAppKeepAlive(process, response, sizeof(response));
             break;
            case CALCAPP_KEEPALIVE_ACK:
               handleCalcAppKeepAliveAck(process, response, sizeof(response));
             break;
            case CALCAPP_COMPLETE:
               handleCalcAppCompleted(process, response, sizeof(response));
             break;
            default:
               cerr << "ERROR: Unknown message type " << ntohl(response->Type) << endl;
             break;
         }
      }
   }
}


// ###### Handle timers #####################################################
void handleTimer(Process* process)
{
   unsigned long long now = getMicroTime();

   if(process->KeepAliveTransmissionTimeStamp <= now) {
      handleKeepAliveTransmissionTimer(process);
      process->KeepAliveTransmissionTimeStamp = ~0ULL;
   }

//   if(!process->Closing) {
      if(process->KeepAliveTimeoutTimeStamp <= now) {
         handleKeepAliveTimeoutTimer(process);
         process->KeepAliveTimeoutTimeStamp = ~0ULL;
      }
     if (process->NextJobTimeStamp <= now)
     {
      handleNextJobTimer(process);
     }
//   }

   if(process->NextJobTimeStamp <= now) {
      handleNextJobTimer(process);
   }
}


// ###### Run process #######################################################
void runProcess(const char* poolHandle, const char* objectName, unsigned long long StartTimer)
{
   struct TagItem tags[16];

   tags[0].Tag  = TAG_RspIO_MakeFailover;
   tags[0].Data = 1;
   tags[1].Tag  = TAG_DONE;

   Process process;
   process.NextJobTimeStamp               = getMicroTime() + (unsigned long long)randomExpDouble(JobInterval);
   process.KeepAliveTransmissionTimeStamp = ~0ULL;
   process.KeepAliveTimeoutTimeStamp      = ~0ULL;
   process.Session                        = NULL;
   process.TotalCalcAppRequests           = 0;
   process.TotalCalcAppRejected           = 0;
   process.TotalCalcAppAccepted           = 0;
   process.TotalCalcAppCompleted           = 0;
   handleNextJobTimer(&process);

   process.CurrentJob = process.Queue.dequeue();
   while(process.CurrentJob != NULL) {
      TotalJobsStarted++;
      TotalJobSizeStarted+=process.CurrentJob->JobSize;
      process.Status = PS_Init;

      process.Session = rspCreateSession((unsigned char*)poolHandle, strlen(poolHandle),
                                         NULL, (struct TagItem*)&tags);
      if(process.Session != NULL) {

         sendCalcAppRequest(&process);

         while((process.Status != PS_Failed) &&
               (process.Status != PS_Completed)) {

            /* ====== Call rspSessionSelect() =============================== */
            tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
            tags[0].Data = 1;
            tags[1].Tag  = TAG_DONE;
            struct SessionDescriptor* sessionDescriptorArray[1];
            unsigned int              sessionStatusArray[1];
            sessionDescriptorArray[0] = process.Session;
            sessionStatusArray[0]     = RspSelect_Read;
            unsigned long long now    = getMicroTime();
            unsigned long long nextTimer = now + 500000;
            nextTimer = min(nextTimer, process.NextJobTimeStamp);
            nextTimer = min(nextTimer, process.KeepAliveTransmissionTimeStamp);
            nextTimer = min(nextTimer, process.KeepAliveTimeoutTimeStamp);
            unsigned long long timeout = (nextTimer >= now) ? (nextTimer - now) : 1;
            int result = rspSessionSelect((struct SessionDescriptor**)&sessionDescriptorArray,
                                          (unsigned int*)&sessionStatusArray,
                                          1,
                                          NULL, NULL, 0,
                                          timeout,
                                          (struct TagItem*)&tags);
       // process.Queue.PrintStatistics();
            handleTimer(&process);
	       if (getMicroTime()-StartTimer >= runtime)
   	       {
      			goto finished;

   	       }


            /* ====== Handle results of ext_select() =========================== */
            if((result > 0) && (sessionStatusArray[0] & RspSelect_Read)) {
               handleEvents(&process, sessionStatusArray[0]);
            }

            if(result < 0) {
               if(errno != EINTR) {
                  perror("rspSessionSelect() failed");
               }
               goto finished;
            }
            if(breakDetected()) {
               rspDeleteSession(process.Session, NULL);
               goto finished;
            }

            if(process.Status == PS_Failover) {
               handleFailover(&process);
            }
         }

         rspDeleteSession(process.Session, NULL);
         process.Session = NULL;

         if(process.Status == PS_Completed) {
            delete process.CurrentJob;
            process.CurrentJob = process.Queue.dequeue();
         }
      }

      while(process.CurrentJob == NULL) {
         tags[0].Tag  = TAG_RspSelect_RsplibEventLoop;
         tags[0].Data = 1;
         tags[1].Tag  = TAG_DONE;
         unsigned long long now       = getMicroTime();
         unsigned long long nextTimer = now + 500000;
         nextTimer = min(nextTimer, process.NextJobTimeStamp);
         unsigned long long timeout = (nextTimer >= now) ? (nextTimer - now) : 1;
         int result = rspSessionSelect(NULL, NULL, 0,
                                       NULL, NULL, 0,
                                       timeout,
                                       (struct TagItem*)&tags);
         if(result < 0) {
            if(errno != EINTR) {
               perror("rspSessionSelect() failed");
            }
            goto finished;
         }
         if(breakDetected()) {
            goto finished;
         }
         handleTimer(&process);
         process.CurrentJob = process.Queue.dequeue();
      }

   }

finished:
    /* AverageHandlingDelay  = TotalHandlingDelay / process.TotalCalcAppCompleted;
    AverageHandlingSpeed  = TotalHandlingSpeed / process.TotalCalcAppCompleted;
    AverageJobSize        = TotalJobSize       / process.TotalCalcAppCompleted;
    AverageJobInterval    = TotalJobInterval   / process.TotalCalcAppCompleted;
   fprintf(ScalarFH, "scalar \"%s\" \"Total CalcAppRequests\" %u\n", objectName, process.TotalCalcAppRequests);
   fprintf(ScalarFH, "scalar \"%s\" \"AverageHandlingDelay \" %1.6f\n", objectName, AverageHandlingDelay);
   fprintf(ScalarFH, "scalar \"%s\" \"AverageHandlingSpeed \" %1.6f\n", objectName, AverageHandlingSpeed);
   fprintf(ScalarFH, "scalar \"%s\" \"AverageJobSize       \" %1.6f\n", objectName, AverageJobSize);
   fprintf(ScalarFH, "scalar \"%s\" \"AverageJobInterval   \" %llu\n", objectName, AverageJobInterval); */

   /*fprintf(ScalarFH, "scalar \"%s\" \"HandlingDelay   \"mean=%f min=%f max=%f stddev=%f\n", objectName, stat1.mean(), stat1.minimum(), stat1.maximum(), stat1.stddev()); */

   fprintf(ScalarFH, "scalar \"%s\" \"StartupTime     \"mean=%f \n", objectName, stat1.mean());
   fprintf(ScalarFH, "scalar \"%s\" \"QueueingTime    \"mean=%f \n", objectName, stat2.mean());
   fprintf(ScalarFH, "scalar \"%s\" \"ProcessingTime  \"mean=%f \n", objectName, stat3.mean());
   fprintf(ScalarFH, "scalar \"%s\" \"HandlingDelay   \"mean=%f \n", objectName, stat4.mean());
   fprintf(ScalarFH, "scalar \"%s\" \"HandlingSpeed   \"mean=%f \n", objectName, stat5.mean());
   fprintf(ScalarFH, "scalar \"%s\" \"JobSize         \"mean=%f \n", objectName, stat6.mean());
   fprintf(ScalarFH, "scalar \"%s\" \"JobInterval     \"mean=%f \n", objectName, stat7.mean());

   fprintf(ScalarFH, "scalar \"%s\" \"StartupTime     \"stddev=%f \n", objectName, stat8.stddev());
   fprintf(ScalarFH, "scalar \"%s\" \"QueueingTime    \"stddev=%f \n", objectName, stat9.stddev());
   fprintf(ScalarFH, "scalar \"%s\" \"ProcessingTime  \"stddev=%f \n", objectName, stat10.stddev());
   fprintf(ScalarFH, "scalar \"%s\" \"HandlingDelay   \"stddev=%f \n", objectName, stat11.stddev());
   fprintf(ScalarFH, "scalar \"%s\" \"HandlingSpeed   \"stddev=%f \n", objectName, stat12.stddev());

   fprintf(ScalarFH, "scalar \"%s\" \"Total CalcAppRequests     \"%u \n", objectName, process.TotalCalcAppRequests);
   fprintf(ScalarFH, "scalar \"%s\" \"Total CalcAppAccepts      \"%u \n", objectName, process.TotalCalcAppAccepted);
   fprintf(ScalarFH, "scalar \"%s\" \"Total CalcAppRejects      \"%u \n", objectName, process.TotalCalcAppRejected);

   fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Queued\" %u \n", objectName, TotalJobsQueued);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Started\" %u \n", objectName, TotalJobsStarted);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Jobs Completed \" %u \n", objectName, process.TotalCalcAppCompleted);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Job Size Queued\" %1.6f \n", objectName, TotalJobSizeQueued);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Job Size Started\" %1.6f \n", objectName, TotalJobSizeStarted);
   fprintf(ScalarFH, "scalar \"%s\" \"Total Job Size Completed\" %1.6f \n", objectName, TotalJobSizeCompleted);

   return;
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{

   char* poolHandle     = "CalcAppPool";
   char* objectName     = "scenario.calcapppooluser[0]";
   char* vectorFileName = "calcapppooluser.vec";
   char* scalarFileName = "calcapppooluser.sca";
   int   i;
   int   n;
   unsigned long long StartTimer = getMicroTime();

   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i],"-ph=",4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i], "-object=" ,8))) {
         objectName = (char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-vector=" ,8))) {
         vectorFileName = (char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-scalar=" ,8))) {
         scalarFileName = (char*)&argv[i][8];
      }
      else if(!(strncmp(argv[i], "-jobinterval=" ,13))) {
         JobInterval 	= atol((char*)&argv[i][13]);
      }
      else if(!(strncmp(argv[i], "-jobsize=" ,9))) {
         JobSizeDeclaration 	= atol((char*)&argv[i][9]);
      }
      else if(!(strncmp(argv[i], "-runtime=" ,9))) {
         runtime 	= atol((char*)&argv[i][9]);
      }
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-registrar=" ,11))) {
         /* Process this later */
      }
      else {
         puts("Bad arguments!");
         printf("Usage: %s {-object=object name} {-vector=vector file name} {-scalar=scalar file name} {-registrar=Registrar address(es)} {-ph=Pool handle} {-logfile=file|-logappend=file|-logquiet} {-loglevel=level} {-logcolor=on|off}\n", argv[0]);
         exit(1);
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      exit(1);
   }
#ifndef FAST_BREAK
   installBreakDetector();
#endif

   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-registrar=" ,11))) {
         if(rspAddStaticRegistrar((char*)&argv[n][11]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad registrar setting: %s\n", argv[n]);
            exit(1);
         }
      }
   }


   cout << "CalcApp Pool User - Version 1.0" << endl
        << "-------------------------------" << endl << endl
        << "Object      = " << objectName << endl
        << "Vector File = " << vectorFileName << endl
        << "Scalar File = " << scalarFileName << endl
        << endl;

   VectorFH = fopen(vectorFileName, "w");
   if(VectorFH == NULL) {
      cout << " Unable to open output file " << vectorFileName << endl;
      finishLogging();
   }

   fprintf(VectorFH, "JobID JobSize JobInterval QueueDelay StartupDelay ProcessingDelay HandlingDelay HandlingSpeed\n");

   ScalarFH = fopen(scalarFileName, "w");
   if(ScalarFH == NULL) {
      cout << " Unable to open output file " << scalarFileName << endl;
      finishLogging();
   }
   fprintf(ScalarFH, "run 1 \"scenario\"\n");


   runProcess(poolHandle, objectName, StartTimer);


   fclose(ScalarFH);
   fclose(VectorFH);


   finishLogging();
   rspCleanUp();
   puts("\nTerminated!");
   return(0);
}
