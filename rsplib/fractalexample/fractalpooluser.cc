#include <iostream>
#include <qapplication.h>
#include <qwidget.h>
#include <qmainwindow.h>
#include <qimage.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qstatusbar.h>
#define QT_THREAD_SUPPORT
#include <qthread.h>
#include <complex>

#include "rsplib.h"
#include "rsplib-tags.h"
#include "loglevel.h"
#include "netutilities.h"

#include "fractalgeneratorexample.h"



class FractalPU : public QWidget,
                  public QThread
{
   Q_OBJECT

   public:
   FractalPU(const size_t width,
                    const size_t height,
                    QWidget*     parent = NULL,
                    const char*  name   = NULL);
   ~FractalPU();

   void reset();

   inline unsigned int getPoint(const size_t x, const size_t y) {
      return(Image->pixel(x, y));
   }

   inline void setPoint(const size_t x, const size_t y, const unsigned int value) {
      Image->setPixel(x, y, value);
   }


   public slots:
   void timeoutExpired();


   protected:
   void paintImage(const size_t startY, const size_t endY);
   void resizeEvent(QResizeEvent* resizeEvent);
   void paintEvent(QPaintEvent* paintEvent);


   private:
   virtual void run();
   bool sendParameter();

   enum DataStatus {
      Okay      = 0,
      Finalizer = 1,
      Invalid   = 2
   };
   DataStatus handleData(const FractalGeneratorData* data,
                         const size_t                size);

   bool                      Running;
   QImage*                   Image;
   QTimer*                   TimeoutTimer;

   unsigned char*            PoolHandle;
   size_t                    PoolHandleSize;
   SessionDescriptor*        Session;
   uint32_t                  LastPoolElementID;
   FractalGeneratorParameter Parameter;
   size_t                    Run;
   size_t                    PoolElementUsages;
};


FractalPU::FractalPU(const size_t width,
                     const size_t height,
                     QWidget*     parent,
                     const char*  name)
   : QWidget(parent, name)
{
   Running          = true;
   Parameter.Width  = width;
   Parameter.Height = height;

   setMinimumSize(Parameter.Width, Parameter.Height);
   setMaximumSize(Parameter.Width, Parameter.Height);

   Image  = new QImage(Parameter.Width, Parameter.Height, 32);
   Q_CHECK_PTR(Image);

   TimeoutTimer = new QTimer;
   Q_CHECK_PTR(TimeoutTimer);
   connect(TimeoutTimer, SIGNAL(timeout()), this, SLOT(timeoutExpired()));

   reset();
   show();
   start();
}


FractalPU::~FractalPU()
{
   delete TimeoutTimer;
   TimeoutTimer = NULL;
   delete Image;
   Image = NULL;
}



void FractalPU::reset()
{
   for(int y = 0;y < Image->height();y++) {
      for(int x = 0;x < Image->width();x++) {
         Image->setPixel(x, y, qRgb(255, 255, 255));
      }
   }
}


void FractalPU::resizeEvent(QResizeEvent* resizeEvent)
{
   update();
}


void FractalPU::paintImage(const size_t startY, const size_t endY)
{
   QPainter p;
   p.begin(this);
   p.drawImage(0, startY,
               *Image,
               0, startY,
               Parameter.Width, endY + 1);
   p.end();
}


void FractalPU::paintEvent(QPaintEvent* paintEvent)
{
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


bool FractalPU::sendParameter()
{
   FractalGeneratorParameter parameter;
   TagItem                   tags[16];

   parameter.Width         = htonl(Parameter.Width);
   parameter.Height        = htonl(Parameter.Height);
   parameter.AlgorithmID   = htonl(Parameter.AlgorithmID);
   parameter.MaxIterations = htonl(Parameter.MaxIterations);
   parameter.C1Real        = Parameter.C1Real;
   parameter.C1Imag        = Parameter.C1Imag;
   parameter.C2Real        = Parameter.C2Real;
   parameter.C2Imag        = Parameter.C2Imag;

   tags[0].Tag  = TAG_RspIO_Timeout;
   tags[0].Data = (tagdata_t)2000000;
   tags[1].Tag  = TAG_DONE;
   if(rspSessionWrite(Session, &parameter, sizeof(parameter),
                      (TagItem*)&tags) <= 0) {
      std::cerr << "ERROR: SessionWrite failed!" << std::endl;
      return(false);
   }
   return(true);
}


FractalPU::DataStatus FractalPU::handleData(const FractalGeneratorData* data,
                                            const size_t                size)
{
   if(size < getFractalGeneratorDataSize(0)) {
      return(Invalid);
   }
   size_t p      = 0;
   size_t x      = ntohl(data->StartX);
   size_t y      = ntohl(data->StartY);
   size_t points = ntohl(data->Points);
   if((points == 0) && (x == 0xffffffff) && (y == 0xffffffff)) {
      update();
      return(Finalizer);
   }
   if(size < getFractalGeneratorDataSize(points)) {
      return(Invalid);
   }
   if(x >= Parameter.Width) {
      return(Invalid);
   }
   if(y >= Parameter.Height) {
      return(Invalid);
   }
   if(points > FGD_MAX_POINTS) {
      return(Invalid);
   }
   while(y < Parameter.Height) {
      while(x < Parameter.Width) {
         if(p > points) {
            break;
         }
         const uint32_t point = ntohl(data->Buffer[p]);
         const QColor color(((point + (2 * Run) + PoolElementUsages) % 72) * 5, 255, 255, QColor::Hsv);
         setPoint(x, y, color.rgb());
         p++;

         x++;
      }
      x = 0;
      y++;
   }
   paintImage(ntohl(data->StartY), y);
   return(Okay);
}


void FractalPU::timeoutExpired()
{
   std::cerr << "##### Timeout expired -> Customer is not satisfied with this service!" << std::endl;
}


void FractalPU::run()
{
   char     statusText[128];
   TagItem  tags[16];

   Run                     = 0;
   PoolElementUsages       = 0;
   Parameter.C1Real        = -1.5;
   Parameter.C1Imag        = 1.5;
   Parameter.C2Real        = 1.5;
   Parameter.C2Imag        = -1.5;
   Parameter.MaxIterations = 150;
   Parameter.AlgorithmID   = FGPA_MANDELBROT;

   PoolHandle     = (unsigned char*)"FractalGeneratorPool";
   PoolHandleSize = strlen((const char*)PoolHandle);
   tags[0].Tag = TAG_TuneSCTP_MinRTO;
   tags[0].Data = 250;
   tags[1].Tag = TAG_TuneSCTP_MaxRTO;
   tags[1].Data = 500;
   tags[2].Tag = TAG_TuneSCTP_InitialRTO;
   tags[2].Data = 250;
   tags[3].Tag = TAG_TuneSCTP_Heartbeat;
   tags[3].Data = 100;
   tags[4].Tag = TAG_TuneSCTP_PathMaxRXT;
   tags[4].Data = 3;
   tags[5].Tag = TAG_TuneSCTP_AssocMaxRXT;
   tags[5].Data = 9;
   tags[6].Tag = TAG_DONE;

   std::cerr << "Creating session..." << std::endl;
   for(;;) {
      LastPoolElementID = 0;
      Session = rspCreateSession(PoolHandle, PoolHandleSize, NULL, (TagItem*)&tags);
      if(Session) {
         rspSessionSetCSPStatus(Session, "Sending parameter command...");

         std::cerr << "Sending parameter command..." << std::endl;
         Run++;
         PoolElementUsages = 0;

         if(sendParameter()) {
            TimeoutTimer->start(2000, TRUE);

            FractalGeneratorData data;
            tags[0].Tag  = TAG_RspIO_PE_ID;
            tags[0].Data = 0;
            tags[1].Tag  = TAG_RspIO_Timeout;
            tags[1].Data = (tagdata_t)2000000;
            tags[2].Tag  = TAG_DONE;
            size_t packets = 0;
            ssize_t received = rspSessionRead(Session, &data, sizeof(data), (TagItem*)&tags);
            while(received != 0) {
               if(received > 0) {
                  if(LastPoolElementID != tags[0].Data) {
                     LastPoolElementID = tags[0].Data;
                     PoolElementUsages++;
                  }
                  TimeoutTimer->stop();
                  TimeoutTimer->start(5000, TRUE);
                  switch(handleData(&data, received)) {
                     case Finalizer:
                        goto finish;
                        break;
                      break;
                     case Invalid:
                        std::cerr << "ERROR: Invalid data block received!" << std::endl;
                      break;
                     default:
                        packets++;
                        snprintf((char*)&statusText, sizeof(statusText),
                                 "Processed data packet #%u...", packets);
                        rspSessionSetCSPStatus(Session, statusText);
                      break;
                  }
               }
               received = rspSessionRead(Session, &data, sizeof(data), (TagItem*)&tags);
            }
         }

finish:
         TimeoutTimer->stop();
         rspSessionSetCSPStatus(Session, "Image completed!");

         std::cout << "Image completed!" << std::endl;
         rspDeleteSession(Session);
         Session = NULL;

         usleep(2000000);
      }
   }
}


#include "fractalpooluser.moc"


/* ###### rsplib main loop thread ########################################### */
static bool RsplibThreadStop = false;
static void* rsplibMainLoop(void* args)
{
   struct timeval timeout;
   while(!RsplibThreadStop) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 50000;
      rspSelect(0, NULL, NULL, NULL, &timeout);
   }
   return(NULL);
}


int main(int argc, char** argv)
{
   uint64_t             cspIdentifier     = 0;
   unsigned int         cspReportInterval = 0;
   union sockaddr_union cspReportAddress;
   struct TagItem       tags[16];
   pthread_t            rsplibThread;
   int                  i;

   string2address("127.0.0.1:2960", &cspReportAddress);
   for(i = 1;i < argc;i++) {
      if(!(strncasecmp(argv[i], "-identifier=", 12))) {
         cspIdentifier = CID_COMPOUND(CID_GROUP_POOLUSER, atol((char*)&argv[i][12]));
      }
      else if(!(strncasecmp(argv[i], "-cspreportinterval=", 19))) {
         cspReportInterval = atol((char*)&argv[i][19]);
      }
      else if(!(strncasecmp(argv[i], "-cspreportaddress=", 18))) {
         if(!string2address((char*)&argv[i][18], &cspReportAddress)) {
            fprintf(stderr,
                    "ERROR: Bad CSP report address %s! Use format <address:port>.\n",
                    (char*)&argv[i][18]);
            exit(1);
         }
         if(cspReportInterval <= 0) {
            cspReportInterval = 250000;
         }
      }
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
   }

   beginLogging();
   tags[0].Tag  = TAG_RspLib_CSPReportAddress;
   tags[0].Data = (tagdata_t)&cspReportAddress;
   tags[1].Tag  = TAG_RspLib_CSPReportInterval;
   tags[1].Data = (tagdata_t)cspReportInterval;
   tags[2].Tag  = TAG_RspLib_CSPIdentifier;
   tags[2].Data = (tagdata_t)&cspIdentifier;
   tags[3].Tag  = TAG_DONE;
   if(rspInitialize((struct TagItem*)&tags) != 0) {
      std::cerr << "ERROR: Unable to initialize rsplib!" << std::endl;
      exit(1);
   }
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-nameserver=" ,12))) {
         if(rspAddStaticNameServer((char*)&argv[i][12]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad name server setting: %s\n", argv[i]);
            exit(1);
         }
      }
   }

   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create rsplib main loop thread!");
   }

   QApplication application(argc, argv);
   QMainWindow* mainWindow = new QMainWindow;
   Q_CHECK_PTR(mainWindow);
   FractalPU* fractalPU = new FractalPU(400, 250, mainWindow);
   Q_CHECK_PTR(fractalPU);
   mainWindow->setCaption("Fractal Pool User");
   (mainWindow->statusBar())->message("Welcome to Fractal PU!", 3000);
   mainWindow->setCentralWidget(fractalPU);
   mainWindow->show();
   return(application.exec());
}
