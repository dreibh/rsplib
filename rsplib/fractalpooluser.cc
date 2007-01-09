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

#include <qapplication.h>
#include <qstatusbar.h>
#include <qthread.h>
#include <qpainter.h>
#include <qfile.h>
#include <qstringlist.h>
#include <qdom.h>
#include <iostream>
#include <complex>

#include "tdtypes.h"
#include "fractalpooluser.h"
#include "rserpool-internals.h"
#include "loglevel.h"
#include "netutilities.h"
#include "rsputilities.h"
#include "randomizer.h"
#include "fractalgeneratorpackets.h"
#include "netutilities.h"


using namespace std;


#define DEFAULT_FPU_WIDTH             400
#define DEFAULT_FPU_HEIGHT            250
#define DEFAULT_FPU_SEND_TIMEOUT     5000
#define DEFAULT_FPU_RECV_TIMEOUT     5000
#define DEFAULT_FPU_INTER_IMAGE_TIME    5


/* ###### Constructor #################################################### */
FractalPU::FractalPU(const size_t       width,
                     const size_t       height,
                     const char*        poolHandle,
                     const char*        configDirName,
                     const unsigned int sendTimeout,
                     const unsigned int recvTimeout,
                     const unsigned int interImageTime,
                     const size_t       threads,
                     QWidget*           parent,
                     const char*        name)
   : QMainWindow(parent, name)
{
   Image          = NULL;
   PoolHandle     = (const unsigned char*)poolHandle;
   PoolHandleSize = strlen((const char*)PoolHandle);

   RecvTimeout    = sendTimeout;
   SendTimeout    = recvTimeout;
   InterImageTime = interImageTime;
   Threads        = threads;

   // ====== Initialize file and directory names ============================
   ConfigDirectory = QDir(configDirName);
   if(ConfigDirectory.exists() != TRUE) {
      cerr << "WARNING: Configuration directory " << configDirName << " does not exist!" << endl;
      ConfigDirectory = QDir::current();
   }
   const QString configFileName = ConfigDirectory.filePath("scenarios.conf");

   // ====== Read configuration =============================================
   QFile configFile(configFileName);
   if(!configFile.open( IO_ReadOnly ) ) {
      cerr << "WARNING: Unable to open configuration file "
                << configFileName << "!" << endl;
   }
   else {
      QString buffer;
      while(!configFile.atEnd()) {
         configFile.readLine(buffer, 255);
         buffer = buffer.stripWhiteSpace();
         ConfigList.append(buffer);
      }
   }

   // ====== Initialize widget and PU thread ================================
   resize(width, height);
   setCaption("Fractal Pool User");
   statusBar()->message("Welcome to Fractal PU!", 3000);
   show();
   start();
}


/* ###### Destructor ##################################################### */
FractalPU::~FractalPU()
{
   Running = false;
   wait();
   delete Image;
   Image = NULL;
}


/* ###### Paint fractal image ############################################ */
void FractalPU::paintImage(const size_t startY, const size_t endY)
{
   QPainter p;
   p.begin(this);
   p.drawImage(0, startY,
               *Image,
               0, startY,
               Image->width(), endY + 1 - startY);
   p.end();
}


/* ###### Handle Paint event ############################################# */
void FractalPU::paintEvent(QPaintEvent* paintEvent)
{
   qApp->lock();
   if(Image) {
      QPainter p;
      p.begin(this);
      p.drawImage(paintEvent->rect().left(), paintEvent->rect().top(),
                  *Image,
                  paintEvent->rect().left(),
                  paintEvent->rect().top(),
                  paintEvent->rect().width(),
                  paintEvent->rect().height());
      p.end();
   }
   qApp->unlock();
}


/* ###### Handle Close event ############################################# */
void FractalPU::closeEvent(QCloseEvent* closeEvent)
{
   Running = false;
   qApp->exit(0);
}


/* ###### Get parameters from config file ################################ */
void FractalPU::getNextParameters()
{
   if(ConfigList.count() == 0) {
      // cerr << "Config file list empty" << endl;
      return;
   }
   const size_t element = random32() % ConfigList.count();

   QString parameterFileName(ConfigList[element]);
   QDomDocument doc("XMLFractalSave");
   cout << "Getting parameters from " << ConfigDirectory.filePath(parameterFileName) << "..." << endl;
   QFile parameterFile(ConfigDirectory.filePath(parameterFileName));
   if(!parameterFile.open(IO_ReadOnly)) {
      cerr << "WARNING: Cannot open parameter file " << parameterFileName << "!" << endl;
      return;
   }

   QString error;
   int line, column;
   if(!doc.setContent(&parameterFile, false, &error, &line, &column)) {
      parameterFile.close();
      cerr << "WARNING: Error in parameter file " << parameterFileName << ":" << endl
                << error << " in line " << line << ", column " << column << endl;
      return;
   }
   parameterFile.close();

   QDomElement algorithm = doc.elementsByTagName("AlgorithmName").item(0).toElement();
   const QString algorithmName = algorithm.firstChild().toText().data();
   if(algorithmName == "MandelbrotN") {
      Parameter.AlgorithmID = FGPA_MANDELBROTN;
   }
   else if(algorithmName == "Mandelbrot") {
      Parameter.AlgorithmID = FGPA_MANDELBROT;
   }
   else {
      cerr << "WARNING: Unknown algorithm name in parameter file " << parameterFileName << "!" << endl
           << "Assuming Mandelbrot..." << endl;
      Parameter.AlgorithmID = FGPA_MANDELBROT;
   }

   Parameter.C1Real = doc.elementsByTagName("C1Real").item(0).firstChild().toText().data().toDouble();
   Parameter.C1Imag = doc.elementsByTagName("C1Imag").item(0).firstChild().toText().data().toDouble();
   Parameter.C2Real = doc.elementsByTagName("C2Real").item(0).firstChild().toText().data().toDouble();
   Parameter.C2Imag = doc.elementsByTagName("C2Imag").item(0).firstChild().toText().data().toDouble();

   // Defaults; will be overwritten if specified in the parameter file.
   Parameter.N             = 2.0;
   Parameter.MaxIterations = 123;

   QDomElement userOptions = doc.elementsByTagName("Useroptions").item(0).toElement();
   QDomNode child = userOptions.firstChild();
   while(!child.isNull()) {
      const QString name  = child.nodeName();
      const QString value = child.firstChild().toText().data();
      if(name == "MaxIterations") {
         Parameter.MaxIterations = value.toInt();
      }
      else if(name == "N") {
         Parameter.N = value.toDouble();
      }
      child = child.nextSibling();
   }
}


/* ###### Fractal PU thread implementation ############################### */
void FractalPU::run()
{
   FractalCalculationThread* calculationThread[Threads];

   Run     = 0;
   Running = true;
   while(Running) {
      // ====== Initialize image object and timeout timer ===================
      Run++;
      qApp->lock();
      Parameter.Width         = width();
      Parameter.Height        = height();
      Parameter.C1Real        = -1.5;
      Parameter.C1Imag        = 1.5;
      Parameter.C2Real        = 1.5;
      Parameter.C2Imag        = -1.5;
      Parameter.N             = 2.0;
      Parameter.MaxIterations = 1024;
      Parameter.AlgorithmID   = FGPA_MANDELBROT;
      getNextParameters();
      if(Image == NULL) {
         Image = new QImage(Parameter.Width, Parameter.Height, 32);
         Q_CHECK_PTR(Image);
         Image->fill(qRgb(255, 255, 255));
      }
      qApp->unlock();


      // ====== Start job distribution ======================================
      // cout << "Starting job distribution ..." << endl;
      qApp->lock();
      statusBar()->message("Starting job distribution ...");
      qApp->unlock();
      for(size_t i = 0;i < Threads;i++) {
         calculationThread[i] = new FractalCalculationThread(this, i,
                                       0, i * (Parameter.Height / Threads),
                                       Parameter.Width, Parameter.Height / Threads,
                                       (Threads == 1));
         Q_CHECK_PTR(calculationThread[i]);
         calculationThread[i]->start();
      }


      // ====== Wait for job completion =====================================
      // cout << "Waiting for job completion ..." << endl;
      if(Threads > 1) {
         qApp->lock();
         statusBar()->message("Waiting for job completion ...");
         qApp->unlock();
      }
      size_t failed = 0;
      for(size_t i = 0;i < Threads;i++) {
         calculationThread[i]->wait();
         if(!calculationThread[i]->getSuccess()) {
            failed++;
         }
         delete calculationThread[i];
         calculationThread[i] = NULL;
      }
      qApp->lock();
      statusBar()->message((failed == 0) ? "Image completed!" : "Image calculation failed!");
      qApp->unlock();

      // ====== Pause before next image =====================================
      if(Running) {
         char str[128];
         size_t secsToWait = InterImageTime;
         while(secsToWait > 0) {
            usleep(1000000);
            secsToWait--;
            snprintf((char*)&str, sizeof(str), "Waiting for %u seconds ...", secsToWait);
            qApp->lock();
            statusBar()->message(str);
            qApp->unlock();
         }
      }


      // ====== Clean-up ====================================================
      qApp->lock();
      if( (!Running) ||
         (Image->width() != width()) ||
         (Image->height() != height()) ) {
         // Thread destruction or size change
         delete Image;
         Image = NULL;
      }
      qApp->unlock();
   }
}


/* ###### Fractal PU thread constructor ################################## */
FractalCalculationThread::FractalCalculationThread(FractalPU*         fractalPU,
                                                   const unsigned int threadID,
                                                   const size_t       viewX,
                                                   const size_t       viewY,
                                                   const size_t       viewWidth,
                                                   const size_t       viewHeight,
                                                   const bool         showStatus)

{
   ThreadID   = threadID;
   Master     = fractalPU;
   ViewX      = viewX;
   ViewY      = viewY;
   ViewWidth  = viewWidth;
   ViewHeight = viewHeight;
   ShowStatus = showStatus;
   Session    = -1;
   Success    = false;
}


/* ###### Send Parameter message ######################################### */
bool FractalCalculationThread::sendParameterMessage()
{
   FGPParameter parameter;
   ssize_t      sent;

   parameter.Header.Type   = FGPT_PARAMETER;
   parameter.Header.Flags  = 0x00;
   parameter.Header.Length = htons(sizeof(parameter));
   parameter.Width         = htonl(ViewWidth);
   parameter.Height        = htonl(ViewHeight);
   parameter.AlgorithmID   = htonl(Master->Parameter.AlgorithmID);
   parameter.MaxIterations = htonl(Master->Parameter.MaxIterations);
   parameter.C1Real        = doubleToNetwork(Master->Parameter.C1Real + (double)ViewX * (Master->Parameter.C2Real - Master->Parameter.C1Real) / (double)Master->Parameter.Width);
   parameter.C1Imag        = doubleToNetwork(Master->Parameter.C1Imag + (double)ViewY * (Master->Parameter.C2Imag - Master->Parameter.C1Imag) / (double)Master->Parameter.Height);
   parameter.C2Real        = doubleToNetwork(Master->Parameter.C1Real + (double)(ViewX + ViewWidth) * (Master->Parameter.C2Real - Master->Parameter.C1Real) / (double)Master->Parameter.Width);
   parameter.C2Imag        = doubleToNetwork(Master->Parameter.C1Imag + (double)(ViewY + ViewHeight) * (Master->Parameter.C2Imag - Master->Parameter.C1Imag) / (double)Master->Parameter.Height);
   parameter.N             = doubleToNetwork(Master->Parameter.N);

   sent = rsp_sendmsg(Session, (char*)&parameter, sizeof(parameter), 0,
                      0, htonl(PPID_FGP), 0, 0, Master->SendTimeout);
   if(sent < 0) {
      logerror("rsp_sendmsg() failed");
      return(false);
   }
   return(true);
}


/* ###### Handle Data message ############################################ */
FractalCalculationThread::DataStatus FractalCalculationThread::handleDataMessage(const FGPData* data,
                                                                                 const size_t   size)
{
   if(size < getFGPDataSize(0)) {
      return(Invalid);
   }
   size_t p      = 0;
   size_t x      = ntohl(data->StartX);
   size_t y      = ntohl(data->StartY);
   size_t points = ntohl(data->Points);
   if((points == 0) && (x == 0xffffffff) && (y == 0xffffffff)) {
      // Master->update();
      return(Finalizer);
   }
   if(size < getFGPDataSize(points)) {
      return(Invalid);
   }
   if(x >= ViewWidth) {
      return(Invalid);
   }
   if(y >= ViewHeight) {
      return(Invalid);
   }
   if(points > FGD_MAX_POINTS) {
      return(Invalid);
   }

   while(y < ViewHeight) {
      while(x < ViewWidth) {
         if(p >= points) {
            goto finished;
         }
         const uint32_t point = ntohl(data->Buffer[p]);
         const QColor color(((point + (2 * Master->Run) + (3 * ThreadID) + (5 * PoolElementUsages)) % 72) * 5, 255, 255, QColor::Hsv);
         Master->setPoint(x + ViewX, y + ViewY, color.rgb());
         p++;

         x++;
      }
      x = 0;
      y++;
   }

finished:
   Master->paintImage(ntohl(data->StartY) + ViewY, y + ViewY);
   return(Okay);
}


/* ###### Fractal PU thread implementation ############################### */
void FractalCalculationThread::run()
{
   char statusText[128];

   PoolElementUsages = 0;

   Session = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(Session >= 0) {
      if(rsp_connect(Session, Master->PoolHandle, Master->PoolHandleSize) == 0) {

         // ====== Begin image calculation ==================================
         do {
            if(ShowStatus) {
               rsp_csp_setstatus(Session, 0, "Sending parameter command...");
               // cout << "Sending parameter command..." << endl;
               qApp->lock();
               Master->statusBar()->message("Sending parameter command...");
               qApp->unlock();
            }
            if(sendParameterMessage()) {
               do {

                  // ====== Handle received result chunks ===================
                  size_t         packets = 0;
                  FGPData        data;
                  rsp_sndrcvinfo rinfo;
                  int            flags = 0;
                  ssize_t received;

                  received = rsp_recvfullmsg(Session, (char*)&data, sizeof(data),
                                             &rinfo, &flags, Master->RecvTimeout);
                  while(received > 0) {
                     // ====== Handle notification ==========================
                     if(flags & MSG_RSERPOOL_NOTIFICATION) {
                        union rserpool_notification* notification =
                           (union rserpool_notification*)&data;
                        printTimeStamp(stdout);
                        printf("Notification: ");
                        rsp_print_notification((union rserpool_notification*)&data, stdout);
                        puts("");
                        if(notification->rn_header.rn_type == RSERPOOL_FAILOVER) {
                           if(notification->rn_failover.rf_state == RSERPOOL_FAILOVER_NECESSARY) {
                              break;
                           }
                           else if(notification->rn_failover.rf_state == RSERPOOL_FAILOVER_COMPLETE) {
                              PoolElementUsages++;
                           }
                        }
                     }

                     // ====== Handle Data ==================================
                     else {
                        if((received >= (ssize_t)sizeof(FGPCommonHeader)) &&
                           (data.Header.Type == FGPT_DATA)) {
                           qApp->lock();
                           const DataStatus status = handleDataMessage(&data, received);
                           qApp->unlock();

                           switch(status) {
                              case Finalizer:
                                 Success = true;
                                 goto finish;
                              break;
                              case Invalid:
                                 cerr << "ERROR: Invalid data block received!" << endl;
                              break;
                              default:
                                 packets++;
                                 if(ShowStatus) {
                                    snprintf((char*)&statusText, sizeof(statusText),
                                             "Processed paket #%u; PE is $%08x",
                                             packets, rinfo.rinfo_pe_id);
                                    rsp_csp_setstatus(Session, 0, statusText);
                                    snprintf((char*)&statusText, sizeof(statusText),
                                             "Processed data packet #%u (from PE $%08x)...",
                                             packets, rinfo.rinfo_pe_id);
                                    qApp->lock();
                                    Master->statusBar()->message(statusText);
                                    qApp->unlock();
                                 }
                              break;
                           }
                        }
                        if(!Master->Running) {
                           goto finish;
                        }
                     }

                     flags = 0;
                     received = rsp_recvfullmsg(Session, (char*)&data, sizeof(data),
                                                &rinfo, &flags, Master->RecvTimeout);
                  }
                  if(Success == false) {
                     printTimeStamp(stdout);
                     printf("FAILOVER (cookie=%s)...\n", (rsp_has_cookie(Session)) ? "yes" : "NO!");
                     rsp_forcefailover(Session);
                  }

               } while(rsp_has_cookie(Session));
            }
            else {
               printTimeStamp(stdout);
               printf("FAILOVER AFTER FAILED sendParameterMessage() (cookie=%s)...\n", (rsp_has_cookie(Session)) ? "yes" : "NO!");
               rsp_forcefailover(Session);
            }
         } while(Success == false);

         // ====== Image calculation completed ==============================
         if(ShowStatus) {
            const char* statusText = (Success == true) ? "Image completed!" : "Image calculation failed!";
            rsp_csp_setstatus(Session, 0, statusText);
         }
      }
      else {
         logerror("rsp_connect() failed");
      }
   }
   else {
      logerror("rsp_socket() failed");
   }

finish:
   rsp_close(Session);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   struct rsp_info      info;
   union sockaddr_union asapAnnounceAddress;
   char*                poolHandle     = "FractalGeneratorPool";
   const char*          configDirName  = "fgpconfig";
   const char*          caption        = NULL;
   size_t               threads        = 1;
   int                  width          = DEFAULT_FPU_WIDTH;
   int                  height         = DEFAULT_FPU_HEIGHT;
   unsigned int         sendTimeout    = DEFAULT_FPU_SEND_TIMEOUT;
   unsigned int         recvTimeout    = DEFAULT_FPU_RECV_TIMEOUT;
   unsigned int         interImageTime = DEFAULT_FPU_INTER_IMAGE_TIME;
   unsigned int         identifier;
   int                  i;

   memset(&info, 0, sizeof(info));

#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, 0);
#endif
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
#ifdef ENABLE_CSP
      else if(!(strncmp(argv[i], "-identifier=", 12))) {
         if(sscanf((const char*)&argv[i][12], "0x%x", &identifier) == 0) {
            if(sscanf((const char*)&argv[i][12], "%u", &identifier) == 0) {
               fputs("ERROR: Bad registrar ID given!\n", stderr);
               exit(1);
            }
         }
         info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, identifier);
      }
      else if(!(strncmp(argv[i], "-csp" ,4))) {
         if(initComponentStatusReporter(&info, argv[i]) == false) {
            exit(1);
         }
      }
#endif
      else if(!(strncmp(argv[i],"-configdir=",11))) {
         configDirName = (char*)&argv[i][11];
      }
      else if(!(strncmp(argv[i],"-caption=",9))) {
         caption = (char*)&argv[i][9];
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (char*)&argv[i][12];
      }
      else if(!(strncmp(argv[i], "-width=" ,7))) {
         width = atol((char*)&argv[i][7]);
      }
      else if(!(strncmp(argv[i], "-height=" ,8))) {
         height = atol((char*)&argv[i][8]);
      }
      else if(!(strncmp(argv[i], "-threads=" ,9))) {
         threads = atol((char*)&argv[i][9]);
      }
      else if(!(strncmp(argv[i], "-sendtimeout=" ,13))) {
         sendTimeout = atol((char*)&argv[i][13]);
      }
      else if(!(strncmp(argv[i], "-recvtimeout=" ,13))) {
         recvTimeout = atol((char*)&argv[i][13]);
      }
      else if(!(strncmp(argv[i], "-interimagetime=" ,16))) {
         interImageTime = atol((char*)&argv[i][16]);
      }
      else if(!(strncmp(argv[i], "-registrar=", 11))) {
         if(addStaticRegistrar(&info, (char*)&argv[i][11]) < 0) {
            fprintf(stderr, "ERROR: Bad registrar setting %s\n", argv[i]);
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-asapannounce=", 14))) {
         if(string2address((char*)&argv[i][14], &asapAnnounceAddress) == false) {
            fprintf(stderr, "ERROR: Bad ASAP announce setting %s\n", argv[i]);
            exit(1);
         }
         info.ri_registrar_announce = (struct sockaddr*)&asapAnnounceAddress;
      }
      else {
         printf("ERROR: Bad argument %s\n", argv[i]);
         exit(1);
      }
   }
#ifdef ENABLE_CSP
   if(CID_OBJECT(info.ri_csp_identifier) == 0ULL) {
      info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, random64());
   }
#endif
   if(threads < 1) {
      threads = 1;
   }
   else if(threads > 100) {
      threads = 100;
   }
   if(width < 64) {
      width = 64;
   }
   else if(width > 8192) {
      width = 8192;
   }
   if(height < 64) {
      height = 64;
   }
   else if(height > 4096) {
      height = 4096;
   }


   puts("Fractal Pool User - Version 1.0");
   puts("===============================\n");
   printf("Pool Handle      = %s\n", poolHandle);
   printf("Width            = %u\n", width);
   printf("Height           = %u\n", height);
   printf("Send Timeout     = %u [ms]\n", sendTimeout);
   printf("Receive Timeout  = %u [ms]\n", recvTimeout);
   printf("Inter Image Time = %u [s]\n", interImageTime);
   printf("Threads          = %u\n\n", threads);


   beginLogging();

   if(rsp_initialize(&info) < 0) {
      logerror("Unable to initialize rsplib");
      exit(1);
   }


   QApplication application(argc, argv);
   FractalPU* fractalPU = new FractalPU(width, height, poolHandle, configDirName,
                                        sendTimeout, recvTimeout, interImageTime,
                                        threads);
   Q_CHECK_PTR(fractalPU);
   if(caption) {
      fractalPU->setCaption(caption);
   }
   fractalPU->show();
   const int result = application.exec();
   delete fractalPU;


   puts("\nTerminated!");
   rsp_cleanup();
   finishLogging();
   return(result);
}
