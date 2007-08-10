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
 * Copyright (C) 2002-2007 by Thomas Dreibholz
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

#include <qapplication.h>
#include <qstatusbar.h>
#include <qlayout.h>
#include <qthread.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qfile.h>
#include <qstringlist.h>
#include <qdom.h>
#include <iostream>
#include <complex>

#include "tdtypes.h"
#include "rserpool.h"
#include "fractalpooluser.h"
#include "fractalgeneratorpackets.h"
#include "timeutilities.h"
#include "randomizer.h"
#include "netutilities.h"
#include "loglevel.h"
#include "debug.h"
#ifdef ENABLE_CSP
#include "componentstatuspackets.h"
#endif


using namespace std;


#define DEFAULT_FPU_WIDTH             400
#define DEFAULT_FPU_HEIGHT            250
#define DEFAULT_FPU_SEND_TIMEOUT     5000
#define DEFAULT_FPU_RECV_TIMEOUT     5000
#define DEFAULT_FPU_INTER_IMAGE_TIME    5
#define DEFAULT_FPU_THREADS             1

#define DEFAULT_FPU_CAPTION     NULL
#define DEFAULT_FPU_POOLHANDLE  "FractalGeneratorPool"
#define DEFAULT_FPU_CONFIGDIR   "fgpconfig"
#define DEFAULT_FPU_IMAGEPREFIX ""
#define DEFAULT_FPU_COLORMARKS  TRUE



/* ###### Constructor #################################################### */
FractalPU::FractalPU(const size_t       width,
                     const size_t       height,
                     const char*        poolHandle,
                     const char*        configDirName,
                     const unsigned int sendTimeout,
                     const unsigned int recvTimeout,
                     const unsigned int interImageTime,
                     const char*        imageStoragePrefix,
                     const bool         colorMarks,
                     const size_t       threads,
                     QWidget*           parent,
                     const char*        name)
   : QWidget(parent, name)
{
   QGridLayout* layout = new QGridLayout(this, 2, 1);
   Q_CHECK_PTR(layout);
   Display = new ImageDisplay(this, "Display");
   Q_CHECK_PTR(Display);
   StatusBar = new QStatusBar(this);
   Q_CHECK_PTR(StatusBar);
   Display->setMinimumSize(width, height);
   layout->addWidget(Display, 0, 0);
   layout->addWidget(StatusBar, 1, 0);
   layout->setRowStretch(0, 10);
   layout->setRowStretch(1, 0);
   PoolHandle     = (const unsigned char*)poolHandle;
   PoolHandleSize = strlen((const char*)PoolHandle);

   RecvTimeout        = sendTimeout;
   SendTimeout        = recvTimeout;
   InterImageTime     = interImageTime;
   ImageStoragePrefix = QString(imageStoragePrefix);
   ColorMarks         = colorMarks;
   Threads            = threads;
   FileNumber         = 0;

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
   setCaption("Fractal Pool User");
   StatusBar->message("Welcome to Fractal PU!", 3000);
   show();
   start();
}


/* ###### Destructor ##################################################### */
FractalPU::~FractalPU()
{
   Status = FPU_Shutdown;
   wait();
   delete Display;
   Display = NULL;
}


/* ###### ImageDisplay constructor ####################################### */
ImageDisplay::ImageDisplay(QWidget*    parent,
                           const char* name)
   : QWidget(parent, name)
{
   Image = NULL;
}


/* ###### ImageDisplay destructor ######################################## */
ImageDisplay::~ImageDisplay()
{
   destroy();
}


/* ###### Initialize ImageDisplay ######################################## */
void ImageDisplay::initialize(const size_t width, const size_t height)
{
   destroy();
   if(Image == NULL) {
      Image = new QImage(width, height, 32);
      Q_CHECK_PTR(Image);
      Image->fill(qRgb(200, 200, 200));
   }
}


/* ###### Destroy ImageDisplay ########################################### */
void ImageDisplay::destroy()
{
   if(Image) {
      delete Image;
      Image = NULL;
   }
}


/* ###### Paint ImageDisplay ############################################# */
void ImageDisplay::paintImage(const size_t startY, const size_t endY)
{
   if(Image) {
      QPainter p;
      p.begin(this);
      p.drawImage(0, startY,
                  *Image,
                  0, startY,
                  Image->width(), endY + 1 - startY);
      p.end();
   }
}


/* ###### Handle Paint event ############################################# */
void ImageDisplay::paintEvent(QPaintEvent* paintEvent)
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
   Status = FPU_Shutdown;
   qApp->exit(0);
}


/* ###### Handle Resize event ############################################ */
void FractalPU::resizeEvent(QResizeEvent* resizeEvent)
{
   Status = FPU_CalcAborted;
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
   FractalCalculationThread* calculationThreadArray[Threads];

   Run    = 0;
   while(Status != FPU_Shutdown) {
      // ====== Initialize image object and timeout timer ===================
      qApp->lock();
      Status = FPU_CalcInProgress;
      Run++;
      Display->initialize(Display->width(), Display->height());
      Parameter.Width         = Display->width();
      Parameter.Height        = Display->height();
      Parameter.C1Real        = -1.5;
      Parameter.C1Imag        = 1.5;
      Parameter.C2Real        = 1.5;
      Parameter.C2Imag        = -1.5;
      Parameter.N             = 2.0;
      Parameter.MaxIterations = 1024;
      Parameter.AlgorithmID   = FGPA_MANDELBROT;
      getNextParameters();
      qApp->unlock();


      // ====== Start job distribution ======================================
      // cout << "Starting job distribution ..." << endl;
      qApp->lock();
      setCursor(Qt::WaitCursor);
      StatusBar->message("Starting job distribution ...");

      const size_t yCount = (size_t)floor(sqrt((double)Threads));
      const size_t yStep  = (size_t)rint((double)Parameter.Height / yCount);
      size_t remaining = Threads;
      size_t number    = 0;
      for(size_t yPosition = 0;yPosition < yCount;yPosition++) {
         const size_t xCount = (yPosition < yCount - 1) ? min(remaining, yCount) : remaining;
         if(xCount > 0) {
            const size_t xStep = (size_t)rint(Parameter.Width / xCount);
            remaining -= xCount;
            for(size_t xPosition = 0;xPosition < xCount;xPosition++) {
               if((Threads > 1) && (ColorMarks)) {
                  const QColor color(((5 * number) % 72) * 5, 100, 255, QColor::Hsv);
                  Display->fillRect(xPosition * xStep, yPosition * yStep,
                                    xStep, yStep, color.rgb());
               }

               calculationThreadArray[number] =
                  new FractalCalculationThread(this, number,
                         xPosition * xStep, yPosition * yStep, xStep, yStep,
                         (Threads == 1), ColorMarks);
               Q_CHECK_PTR(calculationThreadArray[number]);
               calculationThreadArray[number]->start();
               number++;
            }
         }
         Display->paintImage(yPosition * yStep, (yPosition + 1) * yStep);
      }
      qApp->unlock();
      CHECK(number == Threads);

      // ====== Wait for job completion =====================================
      // cout << "Waiting for job completion ..." << endl;
      if(Threads > 1) {
         qApp->lock();
         StatusBar->message("Waiting for job completion ...");
         qApp->unlock();
      }
      size_t failed = 0;
      for(size_t i = 0;i < Threads;i++) {
         calculationThreadArray[i]->wait();
         if(!calculationThreadArray[i]->getSuccess()) {
            failed++;
         }
      }
      qApp->lock();
      StatusBar->message((failed == 0) ? "Image completed!" : "Image calculation failed!");
      setCursor(Qt::ArrowCursor);
      for(size_t i = 0;i < Threads;i++) {
         delete calculationThreadArray[i];   // Must be called with qApp locked!
         calculationThreadArray[i] = NULL;
      }
      qApp->unlock();

      // ====== Save image ==================================================
      if((failed == 0) && (ImageStoragePrefix != "")) {
         QString fileName;
         do {
            FileNumber++;
            QString fileSuffix;
            fileSuffix.sprintf("-%06u.png", (unsigned int)FileNumber);
            fileName = ImageStoragePrefix + fileSuffix;
         } while((QFile::exists(fileName)) && (FileNumber < 999999));
         StatusBar->message("Storing image as " + fileName);
         if(!Display->Image->save(fileName, "PNG")) {
            StatusBar->message("Storing image as " + fileName+ " failed!");
         }
         else {
            StatusBar->message("Storing image as " + fileName+ " stored");
         }
      }

      // ====== Pause before next image =====================================
      if(Status == FPU_CalcInProgress) {
         char str[128];
         size_t secsToWait = InterImageTime;
         while(secsToWait > 0) {
            usleep(1000000);
            secsToWait--;
            snprintf((char*)&str, sizeof(str), "Waiting for %u seconds ...", (int)secsToWait);
            qApp->lock();
            StatusBar->message(str);
            qApp->unlock();
         }
      }
      else if(Status == FPU_CalcAborted) {
         qApp->lock();
         StatusBar->message("Restarting ...");
         qApp->unlock();
         usleep(500000);
      }
   }
}


/* ###### Fractal PU thread constructor ################################## */
FractalCalculationThread::FractalCalculationThread(FractalPU*         fractalPU,
                                                   const unsigned int threadID,
                                                   const size_t       viewX,
                                                   const size_t       viewY,
                                                   const size_t       viewWidth,
                                                   const size_t       viewHeight,
                                                   const bool         showStatus,
                                                   const bool         colorMarks)

{
   ThreadID   = threadID;
   Master     = fractalPU;
   ViewX      = viewX;
   ViewY      = viewY;
   ViewWidth  = viewWidth;
   ViewHeight = viewHeight;
   ShowStatus = showStatus;
   ColorMarks = colorMarks;
   Session    = -1;
   Success    = false;
}


/* ###### Send Parameter message ######################################### */
bool FractalCalculationThread::sendParameterMessage()
{
   FGPParameter parameter;
   ssize_t      sent;

   const double c1real = Master->Parameter.C1Real + (double)ViewX * (Master->Parameter.C2Real - Master->Parameter.C1Real) / (double)Master->Parameter.Width;
   const double c1imag = Master->Parameter.C1Imag + (double)ViewY * (Master->Parameter.C2Imag - Master->Parameter.C1Imag) / (double)Master->Parameter.Height;
   const double c2real = Master->Parameter.C1Real + (double)(ViewX + ViewWidth) * (Master->Parameter.C2Real - Master->Parameter.C1Real) / (double)Master->Parameter.Width;
   const double c2imag = Master->Parameter.C1Imag + (double)(ViewY + ViewHeight) * (Master->Parameter.C2Imag - Master->Parameter.C1Imag) / (double)Master->Parameter.Height;

   parameter.Header.Type   = FGPT_PARAMETER;
   parameter.Header.Flags  = 0x00;
   parameter.Header.Length = htons(sizeof(parameter));
   parameter.Width         = htonl(ViewWidth);
   parameter.Height        = htonl(ViewHeight);
   parameter.AlgorithmID   = htonl(Master->Parameter.AlgorithmID);
   parameter.MaxIterations = htonl(Master->Parameter.MaxIterations);
   parameter.C1Real        = doubleToNetwork(c1real);
   parameter.C1Imag        = doubleToNetwork(c1imag);
   parameter.C2Real        = doubleToNetwork(c2real);
   parameter.C2Imag        = doubleToNetwork(c2imag);
   parameter.N             = doubleToNetwork(Master->Parameter.N);

   sent = rsp_sendmsg(Session, (char*)&parameter, sizeof(parameter), 0,
                      0, htonl(PPID_FGP), 0, 0, 0, Master->SendTimeout);
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
         uint32_t point = ntohl(data->Buffer[p]);
         if(ColorMarks) {
            point = (point + (2 * Master->Run) + (3 * ThreadID) + (5 * PoolElementUsages) % 72) * 5;
         }
         else {
            point = (point % 72) * 5;
         }
         const QColor color(point, 255, 255, QColor::Hsv);
         Master->Display->setPixel(x + ViewX, y + ViewY, color.rgb());
         p++;

         x++;
      }
      x = 0;
      y++;
   }

finished:
   Master->Display->paintImage(ntohl(data->StartY) + ViewY, y + ViewY);
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
               Master->StatusBar->message("Sending parameter command...");
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
                                             "Processed packet #%u; PE is $%08x",
                                             (unsigned int)packets, rinfo.rinfo_pe_id);
                                    rsp_csp_setstatus(Session, 0, statusText);
                                    snprintf((char*)&statusText, sizeof(statusText),
                                             "Processed data packet #%u (from PE $%08x)...",
                                             (unsigned int)packets, rinfo.rinfo_pe_id);
                                    qApp->lock();
                                    Master->StatusBar->message(statusText);
                                    qApp->unlock();
                                 }
                              break;
                           }
                        }
                     }

                     if(Master->Status != FractalPU::FPU_CalcInProgress) {
                        goto finish;
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
   struct rsp_info info;
   const char*     caption            = DEFAULT_FPU_CAPTION;
   size_t          threads            = DEFAULT_FPU_THREADS;
   const char*     poolHandle         = DEFAULT_FPU_POOLHANDLE;
   const char*     configDirName      = DEFAULT_FPU_CONFIGDIR;
   int             width              = DEFAULT_FPU_WIDTH;
   int             height             = DEFAULT_FPU_HEIGHT;
   unsigned int    sendTimeout        = DEFAULT_FPU_SEND_TIMEOUT;
   unsigned int    recvTimeout        = DEFAULT_FPU_RECV_TIMEOUT;
   unsigned int    interImageTime     = DEFAULT_FPU_INTER_IMAGE_TIME;
   const char*     imageStoragePrefix = DEFAULT_FPU_IMAGEPREFIX;
   bool            colorMarks         = DEFAULT_FPU_COLORMARKS;
   unsigned int    identifier;
   int             i;

   rsp_initinfo(&info);
#ifdef ENABLE_CSP
   info.ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLUSER, 0);
#endif
   for(i = 1;i < argc;i++) {
      if(rsp_initarg(&info, argv[i])) {
         /* rsplib argument */
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
#endif
      else if(!(strncmp(argv[i],"-configdir=",11))) {
         configDirName = (const char*)&argv[i][11];
      }
      else if(!(strncmp(argv[i],"-caption=",9))) {
         caption = (const char*)&argv[i][9];
      }
      else if(!(strncmp(argv[i], "-poolhandle=" ,12))) {
         poolHandle = (const char*)&argv[i][12];
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
      else if(!(strncmp(argv[i], "-imagestorageprefix=" ,20))) {
         imageStoragePrefix = (const char*)&argv[i][20];
      }
      else if(!(strcmp(argv[i], "-colormarks"))) {
         colorMarks = TRUE;
      }
      else if(!(strcmp(argv[i], "-nocolormarks"))) {
         colorMarks = FALSE;
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
   else if(threads > 512) {
      threads = 512;
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
   printf("Pool Handle          = %s\n", poolHandle);
   printf("Width                = %u\n", width);
   printf("Height               = %u\n", height);
   printf("Send Timeout         = %u [ms]\n", sendTimeout);
   printf("Receive Timeout      = %u [ms]\n", recvTimeout);
   printf("Inter Image Time     = %u [s]\n", interImageTime);
   printf("Image Storage Prefix = %s\n", imageStoragePrefix);
   printf("Color Marks          = %s\n", (colorMarks == TRUE) ? "on" : "off");
   printf("Threads              = %u\n\n", (unsigned int)threads);


   if(rsp_initialize(&info) < 0) {
      logerror("Unable to initialize rsplib");
      exit(1);
   }


   QApplication application(argc, argv);
   FractalPU* fractalPU = new FractalPU(width, height, poolHandle, configDirName,
                                        sendTimeout, recvTimeout, interImageTime,
                                        imageStoragePrefix, colorMarks, threads);
   Q_CHECK_PTR(fractalPU);
   if(caption) {
      fractalPU->setCaption(caption);
   }
   fractalPU->show();
   const int result = application.exec();
   delete fractalPU;


   puts("\nTerminated!");
   rsp_cleanup();
   rsp_freeinfo(&info);
   return(result);
}
