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

#include <QApplication>
#include <QStatusBar>
#include <QMenu>
#include <QMessageBox>
#include <QLayout>
#include <QThread>
#include <QTimer>
#include <QPainter>
#include <QCursor>
#include <QFile>
#include <QStringList>
#include <QPaintEvent>
#include <QDomDocument>
#include <QMutex>
#include <iostream>
#include <complex>

#include "fractalpooluser.h"

#ifdef FRACTALPOOLUSER_USE_KDE
#include <kcmdlineargs.h>
#include <kstatusbar.h>
#include <kaboutdata.h>
#include <klocale.h>
#endif

#include "tdtypes.h"
#include "rserpool.h"
#include "fractalgeneratorpackets.h"
#include "timeutilities.h"
#include "randomizer.h"
#include "netutilities.h"
#include "loglevel.h"
#include "debug.h"

#include "fractalpooluser_moc.cc"



#define DEFAULT_FPU_WIDTH             400
#define DEFAULT_FPU_HEIGHT            250
#define DEFAULT_FPU_SEND_TIMEOUT     5000
#define DEFAULT_FPU_RECV_TIMEOUT     5000
#define DEFAULT_FPU_INTER_IMAGE_TIME    5
#define DEFAULT_FPU_THREADS             1

#define DEFAULT_FPU_CAPTION          NULL
#define DEFAULT_FPU_POOLHANDLE       "FractalGeneratorPool"
#ifndef DEFAULT_FPU_CONFIGDIR
#define DEFAULT_FPU_CONFIGDIR        "fgpconfig"
#endif
#define DEFAULT_FPU_IMAGEPREFIX      ""
#define DEFAULT_FPU_SHOWCOLORMARKS   TRUE
#define DEFAULT_FPU_SHOWSESSIONS     TRUE



/* ###### Constructor #################################################### */
FractalPU::FractalPU(const size_t       width,
                     const size_t       height,
                     const char*        poolHandle,
                     const char*        configDirName,
                     const unsigned int sendTimeout,
                     const unsigned int recvTimeout,
                     const unsigned int interImageTime,
                     const char*        imageStoragePrefix,
                     const bool         showFailoverMarks,
                     const bool         showSessions,
                     const size_t       threads,
                     QWidget*           parent)
#ifdef FRACTALPOOLUSER_USE_KDE
   : KMainWindow(parent)
#else
   : QMainWindow(parent)
#endif
{
   Display = new ImageDisplay(this);
   Q_CHECK_PTR(Display);
   Display->setMinimumSize(width, height);
   setCentralWidget(Display);

   PoolHandle         = (const unsigned char*)poolHandle;
   PoolHandleSize     = strlen((const char*)PoolHandle);
   RecvTimeout        = sendTimeout;
   SendTimeout        = recvTimeout;
   InterImageTime     = interImageTime;
   ImageStoragePrefix = QString(imageStoragePrefix);
   ShowFailoverMarks     = showFailoverMarks;
   ShowSessions       = showSessions;
   ConfiguredThreads  = threads;
   FileNumber         = 0;
   Run                = 0;

   // ====== Initialize file and directory names ============================
   ConfigDirectory = QDir(configDirName, "*.fsf", QDir::Name, QDir::Files);
   if(ConfigDirectory.exists() != TRUE) {
      std::cerr << "WARNING: Configuration directory " << configDirName << " does not exist!" << std::endl;
      ConfigDirectory = QDir::current();
   }
   else {
      QStringList contents = ConfigDirectory.entryList();
      for(QStringList::Iterator iterator = contents.begin();iterator != contents.end();++iterator) {
         // std::cout << *iterator << std::endl;
         ConfigList.append(*iterator);
      }
   }

   // ====== Initialize widget and PU thread ================================
   setWindowTitle("Fractal Pool User");
   statusBar()->showMessage("Welcome to Fractal PU!", 3000);
   show();
   QTimer::singleShot(100, this, SLOT(startNextJob()));
}


/* ###### Destructor ##################################################### */
FractalPU::~FractalPU()
{
   Status = FPU_Shutdown;
   if(CalculationThreadArray) {
      for(size_t i = 0;i < CurrentThreads;i++) {
         CalculationThreadArray[i]->wait();
         delete CalculationThreadArray[i];
         CalculationThreadArray[i] = NULL;
      }
      delete [] CalculationThreadArray;
      CalculationThreadArray = NULL;
   }
   delete Display;
   Display = NULL;
   CurrentThreads = 0;
}


/* ###### Handle Close event ############################################# */
void FractalPU::closeEvent(QCloseEvent* closeEvent)
{
   quit();
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
   std::cout << "Getting parameters from " << (const char*)(ConfigDirectory.filePath(parameterFileName).toLocal8Bit().constData()) << "..." << std::endl;
   QFile parameterFile(ConfigDirectory.filePath(parameterFileName));
   if(!parameterFile.open(QIODevice::ReadOnly)) {
      std::cerr << "WARNING: Cannot open parameter file " << (const char*)(parameterFileName.toLocal8Bit().constData()) << "!" << std::endl;
      return;
   }

   QString error;
   int line, column;
   if(!doc.setContent(&parameterFile, false, &error, &line, &column)) {
      parameterFile.close();
      std::cerr << "WARNING: Error in parameter file " << (const char*)(parameterFileName.toLocal8Bit().constData()) << ":" << std::endl
                << (const char*)(error.toLocal8Bit().constData()) << " in line " << line << ", column " << column << std::endl;
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
      std::cerr << "WARNING: Unknown algorithm name in parameter file " << (const char*)(parameterFileName.toLocal8Bit().constData()) << "!" << std::endl
                << "Assuming Mandelbrot..." << std::endl;
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


/* ###### Start next fractal PU job ###################################### */
void FractalPU::startNextJob()
{
   CHECK(Status != FPU_CalcInProgress);

   // ====== Initialize image object and timeout timer ======================
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


   // ====== Start job distribution =========================================
   Status = FPU_CalcInProgress;

   // std::cout << "Starting job distribution ..." << std::endl;
   Display->setCursor(Qt::WaitCursor);
   statusBar()->showMessage("Starting job distribution ...");

   CurrentThreads = ConfiguredThreads;
   CalculationThreadArray = new FractalCalculationThread*[CurrentThreads];
   Q_CHECK_PTR(CalculationThreadArray);
   for(size_t i = 0;i < CurrentThreads;i++) {
      CalculationThreadArray[i] = NULL;
   }


   // ====== Initialize configured number of sessions =======================
   QColor       color;
   const size_t yCount = (size_t)floor(sqrt((double)CurrentThreads));
   const size_t yStep  = (size_t)rint((double)Parameter.Height / yCount);

   size_t remaining = CurrentThreads;
   size_t number    = 0;
   for(size_t yPosition = 0;yPosition < yCount;yPosition++) {
      const size_t xCount = (yPosition < yCount - 1) ? min(remaining, yCount) : remaining;
      if(xCount > 0) {
         const size_t xStep = (size_t)rint(Parameter.Width / xCount);
         remaining -= xCount;

         // ====== Initialize rectange for next session =====================
         const QPen numberPen(Qt::red);
         QPainter   painter;
         painter.begin(Display->Image);
         painter.setPen(numberPen);
         for(size_t xPosition = 0;xPosition < xCount;xPosition++) {
            // ====== Mark rectange for next session ========================
            if(CurrentThreads > 1) {
               if(ShowFailoverMarks) {
                  color.setHsv((((5 * number) % 72) * 5) % 256, 100, 255);
                  Display->fillRect(xPosition * xStep, yPosition * yStep,
                                    xStep, yStep, color.rgb());
               }
               char str[64];
               snprintf((char*)&str, sizeof(str), "%u", (unsigned int)number + 1);
               const QString numberString(str);
               painter.drawText(xPosition * xStep, yPosition * yStep, xStep, yStep, Qt::AlignCenter, numberString);
            }

            // ====== Start new session =====================================
            CalculationThreadArray[number] =
               new FractalCalculationThread(this, number,
                      xPosition * xStep, yPosition * yStep, xStep, yStep,
                      (CurrentThreads == 1));
            Q_CHECK_PTR(CalculationThreadArray[number]);

            connect(CalculationThreadArray[number], SIGNAL(updateImage(int, int)),
                    this, SLOT(redrawImage(int, int)));
            connect(CalculationThreadArray[number], SIGNAL(updateStatus(QString)),
                    this, SLOT(changeStatus(QString)));
            connect(CalculationThreadArray[number], SIGNAL(finished()),
                    this, SLOT(handleCompletedSession()));

            CalculationThreadArray[number]->start();
            number++;
         }
         painter.end();
      }
   }
   CHECK(number == CurrentThreads);

   // ====== Make everything visible ========================================
   Display->update();

   // ====== Wait for job completion ========================================
   // std::cout << "Waiting for job completion ..." << std::endl;
   if(CurrentThreads > 1) {
      statusBar()->showMessage("Waiting for job completion ...");
   }
}


/* ###### Session has completed ########################################## */
void FractalPU::handleCompletedSession()
{
   if(CalculationThreadArray) {
      // ====== Count active sessions =======================================
      size_t active = 0;
      for(size_t i = 0;i < CurrentThreads;i++) {
         if( (CalculationThreadArray[i] != NULL) && (!CalculationThreadArray[i]->isFinished()) ) {
            active++;
         }
      }

      // ====== All sessions have finished ==================================
      if(active == 0) {
         Display->setCursor(Qt::ArrowCursor);

         Success = true;
         for(size_t i = 0;i < CurrentThreads;i++) {
            CalculationThreadArray[i]->wait();
            if(!CalculationThreadArray[i]->getSuccess()) {
               Success = false;
            }
            delete CalculationThreadArray[i];
            CalculationThreadArray[i] = NULL;
         }
         delete [] CalculationThreadArray;
         CalculationThreadArray = NULL;

         // ====== Pause before next image ==================================
         if(Status == FPU_CalcInProgress) {
            Status = FPU_Completed;
            CountDown = InterImageTime;
            QTimer::singleShot(0, this, SLOT(countDown()));
         }
         else if(Status == FPU_CalcAborted) {
            Status = FPU_Completed;
            statusBar()->showMessage("Restarting ...");
            QTimer::singleShot(500, this, SLOT(startNextJob()));
         }
      }
   }
}


/* ###### Wait until start of next job ################################### */
void FractalPU::countDown()
{
   char str[128];

   snprintf((char*)&str, sizeof(str),
            "%s - Waiting for %u seconds ...",
            (Success == true) ? "Image completed" : "Image calculation failed",
            CountDown);
   statusBar()->showMessage(str);
   if(CountDown > 0) {
      CountDown--;
      QTimer::singleShot(1000, this, SLOT(countDown()));
   }
   else {
      QTimer::singleShot(1000, this, SLOT(startNextJob()));
   }
}


/* ###### Redraw image ################################################### */
void FractalPU::redrawImage(int start, int end)
{
   // Only update the lines which have changed.
   Display->update(0, start, width(), end + 1 - start);
}


/* ###### Change text of status bar ###################################### */
void FractalPU::changeStatus(QString statusText)
{
   statusBar()->showMessage(statusText);
}


/* ###### Context menu ################################################### */
void FractalPU::contextMenuEvent(QContextMenuEvent* event)
{
   QMenu menu(this);

   static unsigned int sessionsEntries[] = {1,
                                            0,
                                            2, 3, 4, 6, 8, 10,
                                            0,
                                            9, 16, 25, 36, 49, 64, 81, 100,
                                            0,
                                            144, 225, 256, 400};

   QAction* showFailovers = menu.addAction("Show Failovers");
   Q_CHECK_PTR(showFailovers);
   showFailovers->setCheckable(true);
   showFailovers->setChecked(ShowFailoverMarks);
   connect(showFailovers, SIGNAL(toggled(bool)), this, SLOT(changeShowFailoverMarks(bool)));

   QAction* showSessions = menu.addAction("Show Sessions");
   Q_CHECK_PTR(showSessions);
   showSessions->setCheckable(true);
   showSessions->setChecked(ShowSessions);
   connect(showSessions, SIGNAL(toggled(bool)), this, SLOT(changeShowSessions(bool)));
   
   QMenu* sessionsMenu = menu.addMenu("Sessions");
   Q_CHECK_PTR(sessionsMenu);
   for(size_t i = 0;i < sizeof(sessionsEntries) / sizeof(sessionsEntries[0]);i++) {
      if(sessionsEntries[i] > 0) {
         char str[64];
         snprintf((char*)&str, sizeof(str), "%u", sessionsEntries[i]);
         QAction* action = sessionsMenu->addAction(str);
         Q_CHECK_PTR(action);
         action->setCheckable(true);
         if(sessionsEntries[i] == ConfiguredThreads) {
            action->setChecked(true);
         }
         action->setData(sessionsEntries[i]);
      }
      else {
         sessionsMenu->addSeparator();
      }
   }
   connect(sessionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(changeThreads(QAction*)));

   menu.addSeparator();
   menu.addAction("About", this, SLOT(about()));
   menu.addSeparator();
   menu.addAction("Quit", this, SLOT(quit()));

   menu.exec(event->globalPos());
}


/* ###### Change FailoverMarks setting ###################################### */
void FractalPU::changeShowFailoverMarks(bool checked)
{
   ShowFailoverMarks = checked;
}


/* ###### Change FailoverMarks setting ###################################### */
void FractalPU::changeShowSessions(bool checked)
{
   ShowSessions = checked;
}


/* ###### Change Threads setting ######################################### */
void FractalPU::changeThreads(QAction* action)
{
   const unsigned int newThreads = action->data().toInt();
   if(newThreads >= 1) {
      ConfiguredThreads = newThreads;
      Status = FPU_CalcAborted;
   }
}


/* ###### About ########################################################## */
void FractalPU::about()
{
   QMessageBox::about(this, "About Fractal Pool User",
      "<center><b>Fractal Pool User</b><br>Copyright (C) 2002-2009 by Thomas Dreibholz</center>");
}


/* ###### Quit ########################################################### */
void FractalPU::quit()
{
   Status = FPU_Shutdown;
   qApp->exit(0);
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
   return(sent > 0);
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

   Master->Display->lock();
   QColor color;
   while(y < ViewHeight) {
      while(x < ViewWidth) {
         if(p >= points) {
            goto finished;
         }
         uint32_t point = ntohl(data->Buffer[p]);
         if(Master->ShowFailoverMarks) {
            point += (2 * Master->Run) + (5 * PoolElementUsages);
         }
         if(Master->ShowSessions) {
            point += (3 * ThreadID);
         }
         point = ((point % 72) * 5) % 360;
         color.setHsv(point, 255, 255);
         Master->Display->setPixel(x + ViewX, y + ViewY, color.rgb());
         p++;

         x++;
      }
      x = 0;
      y++;
   }

finished:
   Master->Display->unlock();
   emit updateImage(ntohl(data->StartY) + ViewY, y + ViewY);
   return(Okay);
}


/* ###### Fractal PU thread implementation ############################### */
void FractalCalculationThread::run()
{
   char statusText[128];

   PoolElementUsages = 0;

   Session = rsp_socket(0, SOCK_SEQPACKET, IPPROTO_SCTP);
   if(Session >= 0) {
      if(rsp_connect(Session, Master->PoolHandle, Master->PoolHandleSize, 0) == 0) {

         // ====== Begin image calculation ==================================
         do {
            if(ShowStatus) {
               rsp_csp_setstatus(Session, 0, "Sending parameter command ...");
               // cout << "Sending parameter command ..." << endl;
               emit updateStatus("Sending parameter command ...");
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
                           const DataStatus status = handleDataMessage(&data, received);

                           switch(status) {
                              case Finalizer:
                                 Success = true;
                                 goto finish;
                              break;
                              case Invalid:
                                 std::cerr << "ERROR: Invalid data block received!" << std::endl;
                              break;
                              default:
                                 packets++;
                                 if(ShowStatus) {
                                    snprintf((char*)&statusText, sizeof(statusText),
                                             "Processed packet #%03u; PE is $%08x",
                                             (unsigned int)packets, rinfo.rinfo_pe_id);
                                    rsp_csp_setstatus(Session, 0, statusText);
                                    snprintf((char*)&statusText, sizeof(statusText),
                                             "Processed packet #%03u from PE $%08x ...",
                                             (unsigned int)packets, rinfo.rinfo_pe_id);
                                    emit updateStatus(QString(statusText));
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
                     rsp_forcefailover(Session, FFF_NONE, 0);
                  }

               } while(rsp_has_cookie(Session));
            }
            else {
               printTimeStamp(stdout);
               printf("FAILOVER AFTER FAILED sendParameterMessage() (cookie=%s)...\n", (rsp_has_cookie(Session)) ? "yes" : "NO!");
               rsp_forcefailover(Session, FFF_UNREACHABLE, 0);
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



/* ###### Imaay constructor ####################################### */
ImageDisplay::ImageDisplay(QWidget* parent)
   : QWidget(parent)
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
      Image = new QImage(width, height, QImage::Format_RGB32);
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


/* ###### Handle Paint event ############################################# */
void ImageDisplay::paintEvent(QPaintEvent* paintEvent)
{
   if(Image) {
      ImageMutex.lock();
      QPainter p;
      p.begin(this);
      p.drawImage(paintEvent->rect().left(), paintEvent->rect().top(),
                  *Image,
                  paintEvent->rect().left(),
                  paintEvent->rect().top(),
                  paintEvent->rect().width(),
                  paintEvent->rect().height());
      p.end();
      ImageMutex.unlock();
   }
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
   bool            showFailoverMarks  = DEFAULT_FPU_SHOWCOLORMARKS;
   bool            showSessions       = DEFAULT_FPU_SHOWSESSIONS;
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
      else if(!(strcmp(argv[i], "-sessions"))) {
         showSessions = TRUE;
      }
      else if(!(strcmp(argv[i], "-nosessions"))) {
         showSessions = TRUE;
      }
      else if(!(strcmp(argv[i], "-colormarks"))) {
         showFailoverMarks = TRUE;
      }
      else if(!(strcmp(argv[i], "-nocolormarks"))) {
         showFailoverMarks = FALSE;
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


   puts("Fractal Pool User - Version 2.0");
   puts("===============================\n");
   printf("Pool Handle          = %s\n", poolHandle);
   printf("Width                = %u\n", width);
   printf("Height               = %u\n", height);
   printf("Config Directory     = %s\n", configDirName);
   printf("Send Timeout         = %u [ms]\n", sendTimeout);
   printf("Receive Timeout      = %u [ms]\n", recvTimeout);
   printf("Inter Image Time     = %u [s]\n", interImageTime);
   printf("Image Storage Prefix = %s\n", imageStoragePrefix);
   printf("Show Color Marks     = %s\n", (showFailoverMarks == TRUE) ? "on" : "off");
   printf("Show Sessions        = %s\n", (showSessions == TRUE) ? "on" : "off");
   printf("Threads              = %u\n\n", (unsigned int)threads);


   if(rsp_initialize(&info) < 0) {
      logerror("Unable to initialize rsplib");
      exit(1);
   }


#ifdef FRACTALPOOLUSER_USE_KDE
   KAboutData about("fractalpooluser", "fractalpooluser",
                    ki18n("Fractal Pool User"),
                    "2.0kde",
                    ki18n("A KDE/RSerPool Fractal Generator Pool User"),
                    KAboutData::License_GPL_V3,
                    ki18n("Copyright (C) 2009 Thomas Dreibholz"));
   KCmdLineArgs::init(argc, argv, &about);
   KApplication application;
#else
   QApplication application(argc, argv);
#endif
   FractalPU* fractalPU = new FractalPU(width, height, poolHandle, configDirName,
                                        sendTimeout, recvTimeout, interImageTime,
                                        imageStoragePrefix,
                                        showFailoverMarks, showSessions, threads);
   Q_CHECK_PTR(fractalPU);
   if(caption) {
      fractalPU->setWindowTitle(caption);
   }
   fractalPU->show();
   const int result = application.exec();
   delete fractalPU;


   puts("\nTerminated!");
   rsp_cleanup();
   rsp_freeinfo(&info);
   return(result);
}
