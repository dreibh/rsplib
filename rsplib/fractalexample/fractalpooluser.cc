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
#include <qmutex.h>
#include <qfile.h>
#include <complex>
#include <qstringlist.h>
#include <qdom.h>

#include "rsplib.h"
#include "randomizer.h"
#ifdef ENABLE_CSP
#include "componentstatusprotocol.h"
#endif
#include "loglevel.h"
#include "netutilities.h"

#include "fractalgeneratorexample.h"



class FractalPU : public QMainWindow,
                  public QThread,
                  public QMutex
{
   Q_OBJECT

   public:
   FractalPU(const size_t width,
             const size_t height,
             const char*  poolHandle,
             const char*  configDirName,
             QWidget*     parent = NULL,
             const char*  name   = NULL);
   ~FractalPU();

   void restartCalculation();

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
   void paintEvent(QPaintEvent* paintEvent);
   void closeEvent(QCloseEvent* closeEvent);


   private:
   virtual void run();
   bool sendParameter();
   void getNextParameters();

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

   const unsigned char*      PoolHandle;
   size_t                    PoolHandleSize;
   SessionDescriptor*        Session;
   uint32_t                  LastPoolElementID;
   FractalGeneratorParameter Parameter;
   size_t                    Run;
   size_t                    PoolElementUsages;

   QStringList               ConfigList;
   QString                   ConfigDirName;
};


FractalPU::FractalPU(const size_t width,
                     const size_t height,
                     const char*  poolHandle,
                     const char*  configDirName,
                     QWidget*     parent,
                     const char*  name)
   : QMainWindow(parent, name)
{
   Image        = NULL;
   TimeoutTimer = NULL;

   PoolHandle     = (const unsigned char*)poolHandle;
   PoolHandleSize = strlen((const char*)PoolHandle);
   ConfigDirName  = QString(configDirName);

   QString Buffer;
   QFile AllconfigFile("liste.conf");
   if ( !AllconfigFile.open( IO_ReadOnly ) )
   {
      std::cerr << "All-Config file open failed" << std::endl;
   }
   else
   {
      while(AllconfigFile.readLine(Buffer, 255))
      {
         Buffer = Buffer.stripWhiteSpace();
         ConfigList.append(Buffer);
      }
   }

   resize(width, height);
   setCaption("Fractal Pool User");
   statusBar()->message("Welcome to Fractal PU!", 3000);
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



void FractalPU::paintImage(const size_t startY, const size_t endY)
{
   QPainter p;
   p.begin(this);
   p.drawImage(0, startY,
               *Image,
               0, startY,
               Image->width(), endY + 1);
   p.end();
}


void FractalPU::paintEvent(QPaintEvent* paintEvent)
{
   lock();
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
   unlock();
}


void FractalPU::closeEvent(QCloseEvent* closeEvent)
{
   Running = false;
   wait();
   std::cout << "Good bye!" << std::endl;
   qApp->exit(0);
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
   parameter.N             = Parameter.N;

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
   if(x >= (size_t)Image->width()) {
      return(Invalid);
   }
   if(y >= (size_t)Image->height()) {
      return(Invalid);
   }
   if(points > FGD_MAX_POINTS) {
      return(Invalid);
   }
   while(y < (size_t)Image->height()) {
      while(x < (size_t)Image->width()) {
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

void FractalPU::getNextParameters()
{
	if(ConfigList.count() == 0)
	{
		std::cerr << "Config file list empty" << std::endl;
		return;
	}
	size_t Element = random32() % ConfigList.count();


	QString File(ConfigList[Element]);
  	QDomDocument doc( "XMLFractalSave" );
  	QFile file( File);//url.prettyURL().mid(5) );
  	if ( !file.open( IO_ReadOnly ) )
	{
		std::cerr << "Config file open failed" << std::endl;
    		return;
	}

  	QString Error;
  	int Line, Column;
  	if ( !doc.setContent( &file , false, &Error, &Line, &Column) )
	{
    		file.close();
    		std::cerr << "Config file list empty""Fractalgenerator" << Error << " in Line:" << QString().setNum(Line)
       			<< " and Column: " << QString().setNum(Column) << std::endl;
    		return;
  	}
  	file.close();

  	QDomElement algorithm = doc.elementsByTagName("AlgorithmName").item(0).toElement();
  	QString AlgorithmName = algorithm.firstChild().toText().data();
  	if(AlgorithmName == "MandelbrotN")
  	{
  		Parameter.AlgorithmID   = FGPA_MANDELBROTN;
  	}
	else if(AlgorithmName == "Mandelbrot")
  	{
  		Parameter.AlgorithmID   = FGPA_MANDELBROT;
  	}


	Parameter.C1Real        = doc.elementsByTagName("C1Real").item(0).firstChild().toText().data().toDouble();
	Parameter.C1Imag        = doc.elementsByTagName("C1Imag").item(0).firstChild().toText().data().toDouble();
	Parameter.C2Real        = doc.elementsByTagName("C2Real").item(0).firstChild().toText().data().toDouble();
	Parameter.C2Imag        = doc.elementsByTagName("C2Imag").item(0).firstChild().toText().data().toDouble();

  	QDomElement useroptions = doc.elementsByTagName("Useroptions").item(0).toElement();
  	QDomNode child = useroptions.firstChild();
  	while(!child.isNull())
  	{
		QString Name = child.nodeName();
	  	QString Value = child.firstChild().toText().data();
		if(Name == "MaxIterations")
		{
			Parameter.MaxIterations = Value.toInt();
		}
		else if (Name == "N")
		{
			Parameter.N = Value.toInt();
		}
		child = child.nextSibling();
  	}
}


void FractalPU::run()
{
   char              statusText[128];
   TagItem           tags[16];
   Run               = 0;
   PoolElementUsages = 0;

   TimeoutTimer = new QTimer;
   Q_CHECK_PTR(TimeoutTimer);
   connect(TimeoutTimer, SIGNAL(timeout()), this, SLOT(timeoutExpired()));

   std::cerr << "Creating session..." << std::endl;
   for(;;) {
      LastPoolElementID = 0;

      tags[0].Tag  = TAG_TuneSCTP_MinRTO;
      tags[0].Data = 200;
      tags[1].Tag  = TAG_TuneSCTP_MaxRTO;
      tags[1].Data = 500;
      tags[2].Tag  = TAG_TuneSCTP_InitialRTO;
      tags[2].Data = 250;
      tags[3].Tag  = TAG_TuneSCTP_Heartbeat;
      tags[3].Data = 100;
      tags[4].Tag  = TAG_TuneSCTP_PathMaxRXT;
      tags[4].Data = 3;
      tags[5].Tag  = TAG_TuneSCTP_AssocMaxRXT;
      tags[5].Data = 12;
      tags[6].Tag  = TAG_DONE;

      Session = rspCreateSession(PoolHandle, PoolHandleSize, NULL, (TagItem*)&tags);
      if(Session) {
         Run++;
         PoolElementUsages = 0;


         // ====== Initialize image object and timeout timer ================
         lock();
	 Parameter.Width         = width();
         Parameter.Height        = height();
         Parameter.C1Real        = -1.5;
         Parameter.C1Imag        = 1.5;
         Parameter.C2Real        = 1.5;
         Parameter.C2Imag        = -1.5;
         Parameter.MaxIterations = 150;
         Parameter.AlgorithmID   = FGPA_MANDELBROT;
	 getNextParameters();
         if(Image == NULL) {
            Image = new QImage(Parameter.Width, Parameter.Height, 32);
            Q_CHECK_PTR(Image);
            Image->fill(qRgb(255, 255, 255));
         }
         unlock();

         rspSessionSetStatusText(Session, "Sending parameter command...");
         statusBar()->message("Sending parameter command...");
         std::cerr << "Sending parameter command..." << std::endl;


         // ====== Begin image calculation ==================================
         if(sendParameter()) {
            TimeoutTimer->start(2000, TRUE);

            // ====== Handle received result chunks =========================
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
                        rspSessionSetStatusText(Session, statusText);
                        statusBar()->message(statusText);
                      break;
                  }
               }

               if(!Running) {
                  goto finish;
               }

               received = rspSessionRead(Session, &data, sizeof(data), (TagItem*)&tags);
            }
         }

finish:
         // ====== Image calculation completed ==============================
         TimeoutTimer->stop();

         rspSessionSetStatusText(Session, "Image completed!");
         statusBar()->message("Image completed!");
         std::cout << "Image completed!" << std::endl;

         rspDeleteSession(Session);
         Session = NULL;

         if(Running) {
            usleep(2000000);
         }


         // ====== Clean-up =================================================
         lock();
         if( (!Running) ||
             (Image->width() != width()) ||
             (Image->height() != height()) ) {
            // Thread destruction or size change
            delete Image;
            Image = NULL;
         }
         unlock();

         if(!Running) {
            break;
         }
      }
   }

   delete TimeoutTimer;
   TimeoutTimer = NULL;
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
   const char*          poolHandle    = "FractalGeneratorPool";
   const char*          configDirName = NULL;
#ifdef ENABLE_CSP
   struct CSPReporter   cspReporter;
   uint64_t             cspIdentifier     = 0;
   unsigned int         cspReportInterval = 0;
   union sockaddr_union cspReportAddress;
#endif
   pthread_t            rsplibThread;
   int                  i;

   string2address("127.0.0.1:2960", &cspReportAddress);
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i],"-ph=",4))) {
         poolHandle = (char*)&argv[i][4];
      }
      if(!(strncmp(argv[i],"-configdir=",1))) {
         configDirName = (char*)&argv[i][11];
      }
#ifdef ENABLE_CSP
      else if(!(strncasecmp(argv[i], "-identifier=", 12))) {
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
#else
      else if((!(strncmp(argv[i], "-cspreportinterval=", 19))) ||
              (!(strncmp(argv[i], "-cspreportaddress=", 18)))) {
         fprintf(stderr, "ERROR: CSP support not compiled in! Ignoring argument %s\n", argv[i]);
         exit(1);
      }
#endif
      else if(!(strncmp(argv[i],"-log",4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
   }

   beginLogging();
   if(rspInitialize(NULL) != 0) {
      std::cerr << "ERROR: Unable to initialize rsplib!" << std::endl;
      exit(1);
   }
#ifdef ENABLE_CSP
   if((cspReportInterval > 0) && (cspIdentifier != 0)) {
      cspReporterNewForRspLib(&cspReporter, cspIdentifier, &cspReportAddress, cspReportInterval);
   }
#endif
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-registrar=" ,11))) {
         if(rspAddStaticRegistrar((char*)&argv[i][11]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad registrar setting: %s\n", argv[i]);
            exit(1);
         }
      }
   }

   if(pthread_create(&rsplibThread, NULL, &rsplibMainLoop, NULL) != 0) {
      puts("ERROR: Unable to create rsplib main loop thread!");
   }


   QApplication application(argc, argv);
   FractalPU* fractalPU = new FractalPU(400, 250, poolHandle, configDirName);
   Q_CHECK_PTR(fractalPU);
   fractalPU->show();
   const int result = application.exec();


#ifdef ENABLE_CSP
   if(cspReportInterval > 0) {
      cspReporterDelete(&cspReporter);
   }
#endif

   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);

   rspCleanUp();
   finishLogging();
   return(result);
}
