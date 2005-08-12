#define QT_THREAD_SUPPORT
#include <qapplication.h>
#include <qwidget.h>
#include <qmainwindow.h>
#include <qimage.h>
#include <qpainter.h>
#include <qstatusbar.h>
#include <qthread.h>
#include <qfile.h>
#include <complex>
#include <qstringlist.h>
#include <qdom.h>
#include <qdir.h>

#include <iostream>

#include "rsplib.h"
#include "randomizer.h"
#ifdef ENABLE_CSP
#include "componentstatusprotocol.h"
#endif
#include "loglevel.h"
#include "netutilities.h"

#include "fractalgeneratorpackets.h"


using namespace std;


class FractalPU : public QMainWindow,
                  public QThread
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


   protected:
   void paintEvent(QPaintEvent* paintEvent);
   void closeEvent(QCloseEvent* closeEvent);


   private:
   virtual void run();
   bool sendParameter();
   void getNextParameters();
   void paintImage(const size_t startY, const size_t endY);

   enum DataStatus {
      Okay      = 0,
      Finalizer = 1,
      Invalid   = 2
   };
   DataStatus handleData(const FractalGeneratorData* data,
                         const size_t                size);

   bool                      Running;
   QImage*                   Image;

   const unsigned char*      PoolHandle;
   size_t                    PoolHandleSize;
   SessionDescriptor*        Session;
   uint32_t                  LastPoolElementID;
   FractalGeneratorParameter Parameter;
   size_t                    Run;
   size_t                    PoolElementUsages;

   QStringList               ConfigList;
   QDir                      ConfigDirectory;
};


/* ###### Constructor #################################################### */
FractalPU::FractalPU(const size_t width,
                     const size_t height,
                     const char*  poolHandle,
                     const char*  configDirName,
                     QWidget*     parent,
                     const char*  name)
   : QMainWindow(parent, name)
{
   Image          = NULL;
   PoolHandle     = (const unsigned char*)poolHandle;
   PoolHandleSize = strlen((const char*)PoolHandle);

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
   // cout << "Good-bye!" << endl;
   wait(TRUE);
   qApp->exit(0);
}


/* ###### Send Parameter message ######################################### */
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
      cerr << "ERROR: SessionWrite failed!" << endl;
      return(false);
   }
   return(true);
}


/* ###### Handle Data message ############################################ */
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
         if(p >= points) {
            goto finished;
         }
         const uint32_t point = ntohl(data->Buffer[p]);
         const QColor color(((point + (2 * Run) + (5 * PoolElementUsages)) % 72) * 5, 255, 255, QColor::Hsv);
         setPoint(x, y, color.rgb());
         p++;

         x++;
      }
      x = 0;
      y++;
   }

finished:
   paintImage(ntohl(data->StartY), y);
   return(Okay);
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

   const double c1real = doc.elementsByTagName("C1Real").item(0).firstChild().toText().data().toDouble();
   const double c1imag = doc.elementsByTagName("C1Imag").item(0).firstChild().toText().data().toDouble();
   const double c2real = doc.elementsByTagName("C2Real").item(0).firstChild().toText().data().toDouble();
   const double c2imag = doc.elementsByTagName("C2Imag").item(0).firstChild().toText().data().toDouble();

   Parameter.C1Real = doubleToNetwork(c1real);
   Parameter.C1Imag = doubleToNetwork(c1imag);
   Parameter.C2Real = doubleToNetwork(c2real);
   Parameter.C2Imag = doubleToNetwork(c2imag);

   // Defaults; will be overwritten if specified in the parameter file.
   Parameter.N             = doubleToNetwork(2.0);
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
         const double n = value.toDouble();
         Parameter.N = doubleToNetwork(n);
      }
      child = child.nextSibling();
   }
}


/* ###### Fractal PU thread ############################################## */
void FractalPU::run()
{
   char    statusText[128];
   TagItem tags[16];

   Running           = true;
   Run               = 0;
   PoolElementUsages = 0;

   // cerr << "Creating session..." << endl;
   for(;;) {
      LastPoolElementID = 0;

/*
      tags[0].Tag  = TAG_TuneSCTP_MinRTO;
      tags[0].Data = 200;
      tags[1].Tag  = TAG_TuneSCTP_MaxRTO;
      tags[1].Data = 1000;
      tags[2].Tag  = TAG_TuneSCTP_InitialRTO;
      tags[2].Data = 250;
      tags[3].Tag  = TAG_TuneSCTP_Heartbeat;
      tags[3].Data = 100;
      tags[4].Tag  = TAG_TuneSCTP_PathMaxRXT;
      tags[4].Data = 3;
      tags[5].Tag  = TAG_TuneSCTP_AssocMaxRXT;
      tags[5].Data = 12;
      tags[6].Tag  = TAG_DONE;
*/
      tags[0].Tag  = TAG_DONE;

      Session = rspCreateSession(PoolHandle, PoolHandleSize, NULL, (TagItem*)&tags);
      if(Session) {
         Run++;
         PoolElementUsages = 0;


         // ====== Initialize image object and timeout timer ================
         qApp->lock();
         Parameter.Width         = width();
         Parameter.Height        = height();
         Parameter.C1Real        = doubleToNetwork(-1.5);
         Parameter.C1Imag        = doubleToNetwork(1.5);
         Parameter.C2Real        = doubleToNetwork(1.5);
         Parameter.C2Imag        = doubleToNetwork(-1.5);
         Parameter.N             = doubleToNetwork(2.0);
         Parameter.MaxIterations = 1024;
         Parameter.AlgorithmID   = FGPA_MANDELBROT;
         getNextParameters();
         if(Image == NULL) {
            Image = new QImage(Parameter.Width, Parameter.Height, 32);
            Q_CHECK_PTR(Image);
            Image->fill(qRgb(255, 255, 255));
         }
         qApp->unlock();

         rspSessionSetStatusText(Session, "Sending parameter command...");
         // cout << "Sending parameter command..." << endl;
         qApp->lock();
         statusBar()->message("Sending parameter command...");
         qApp->unlock();


         // ====== Begin image calculation ==================================
         bool success = false;
         if(sendParameter()) {

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

                  qApp->lock();
                  const DataStatus status = handleData(&data, received);
                  qApp->unlock();

                  switch(status) {
                     case Finalizer:
                        success = true;
                        goto finish;
                      break;
                     case Invalid:
                        cerr << "ERROR: Invalid data block received!" << endl;
                      break;
                     default:
                        packets++;
                        snprintf((char*)&statusText, sizeof(statusText),
                                 "Processed data packet #%u...", packets);
                        rspSessionSetStatusText(Session, statusText);
                        qApp->lock();
                        statusBar()->message(statusText);
                        qApp->unlock();
                      break;
                  }
               }
               else if(received == RspRead_Timeout) {
                  if(rspSessionHasCookie(Session)) {
                     puts("Timeout occurred -> forcing failover!");
                     rspSessionFailover(Session);
                  }
                  else {
                     puts("No cookie -> restart necessary");
                     goto finish;
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
         const char* statusText = (success == true) ? "Image completed!" : "Image calculation failed!";
         rspSessionSetStatusText(Session, statusText);
         // cout << statusText << endl;
         qApp->lock();
         statusBar()->message(statusText);
         qApp->unlock();

         rspDeleteSession(Session);
         Session = NULL;

         if(Running) {
            usleep(2000000);
         }


         // ====== Clean-up =================================================
         qApp->lock();
         if( (!Running) ||
             (Image->width() != width()) ||
             (Image->height() != height()) ) {
            // Thread destruction or size change
            delete Image;
            Image = NULL;
         }
         qApp->unlock();

         if(!Running) {
            break;
         }
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
   const char*          poolHandle    = "FractalGeneratorPool";
   const char*          configDirName = NULL;
#ifdef ENABLE_CSP
   struct CSPReporter   cspReporter;
   uint64_t             cspIdentifier     = 0;
   unsigned int         cspReportInterval = 333333;
   union sockaddr_union cspReportAddress;
#endif
   pthread_t            rsplibThread;
   int                  i;

   string2address("127.0.0.1:2960", &cspReportAddress);
   for(i = 1;i < argc;i++) {
      if(!(strncmp(argv[i],"-ph=",4))) {
         poolHandle = (char*)&argv[i][4];
      }
      else if(!(strncmp(argv[i],"-configdir=",11))) {
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
      cerr << "ERROR: Unable to initialize rsplib!" << endl;
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
   if((cspReportInterval > 0) && (cspIdentifier != 0)) {
      cspReporterDelete(&cspReporter);
   }
#endif

   RsplibThreadStop = true;
   pthread_join(rsplibThread, NULL);

   rspCleanUp();
   finishLogging();
   return(result);
}
