#include <qapplication.h>
#include <qwidget.h>
#include <qmainwindow.h>
#include <qimage.h>
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
/*
   inline size_t width() const {
      return(Parameter.Width);
   }

   inline size_t height() const {
      return(Parameter.Height);
   }
*/
   protected:
   void paintImage(const size_t startY, const size_t endY);
   void resizeEvent(QResizeEvent* resizeEvent);
   void paintEvent(QPaintEvent* paintEvent);

   private:
   bool sendParameter();
   bool handleData(const FractalGeneratorData* data);

   virtual void run();

   bool                      Running;
   QImage*                   Image;

   unsigned char*            PoolHandle;
   size_t                    PoolHandleSize;
   SessionDescriptor*        Session;
   FractalGeneratorParameter Parameter;
   size_t                    Run;
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
   resize(width, height);
   Image  = new QImage(Parameter.Width, Parameter.Height, 32);
   Q_CHECK_PTR(Image);
   reset();
   start();
}


FractalPU::~FractalPU()
{
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
      puts("ERROR: SessionWrite failed!");
      return(false);
   }
   return(true);
}


bool FractalPU::handleData(const FractalGeneratorData* data)
{
   size_t p      = 0;
   size_t x      = ntohl(data->StartX);
   size_t y      = ntohl(data->StartY);
   size_t points = ntohl(data->Points);
   if(x >= Parameter.Width) {
      return(false);
   }
   if(y >= Parameter.Height) {
      return(false);
   }
   if(points > FGD_MAX_POINTS) {
      return(false);
   }
   while(y < Parameter.Height) {
      while(x < Parameter.Width) {
         if(p > points) {
            break;
         }

         const uint32_t point = ntohl(data->Buffer[p]);

         const QColor color(((point + Run) % 72) * 5, 255, 255, QColor::Hsv);
         setPoint(x, y, color.rgb());
         p++;

         x++;
      }
      x = 0;
      y++;
   }
   paintImage(ntohl(data->StartY), y);
   return(true);
}


void FractalPU::run()
{
   TagItem tags[16];

/*
   for(size_t x = 0;x < Width;x++) {
      for(size_t y = 0;y < Height;y++) {
         setPoint(x, y, qRgb((x*y)%256, x % 256, y % 256));
      }
   }
   update();
*/

puts("start...");
   Run                     = 0;
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
   /*
   tags[6].Tag  = TAG_RspSession_FailoverCallback;
   tags[6].Data = (tagdata_t)handleFailover;
   tags[7].Tag  = TAG_RspSession_FailoverUserData;
   tags[7].Data = (tagdata_t)NULL;
   */
   tags[6].Tag = TAG_DONE;

   puts("Creating session...");
   for(;;) {
      Session = rspCreateSession(PoolHandle, PoolHandleSize, NULL, (TagItem*)&tags);
      if(Session) {
         puts("Sending parameter command...");
         Run++;

         bool success = false;
         do {
            if(sendParameter()) {
               FractalGeneratorData data;

               tags[0].Tag  = TAG_RspIO_Timeout;
               tags[0].Data = (tagdata_t)2000000;
               tags[1].Tag  = TAG_DONE;
               ssize_t received = rspSessionRead(Session, &data, sizeof(data), (TagItem*)&tags);
               for(;;) {
                  if( (received >= (ssize_t)getFractalGeneratorDataSize(0)) &&
                      (received >= (ssize_t)getFractalGeneratorDataSize(ntohl(data.Points))) ) {
                     if(ntohl(data.Points) == 0) {
                        puts("ENDE!");
                        success = true;
                        break;
                     }
                     handleData(&data);
                  }
                  else {
                     printf("received=%d\n",received);
                     if(errno != EAGAIN) {
                        puts("ERROR!");
                        break;
                     }
                     else {
                        puts("eagain...");
                     }
                  }
                  received = rspSessionRead(Session, &data, sizeof(data), (TagItem*)&tags);
   printf("recv=%d\n",received);
               }
            }

            printf("success=%d\n",success);
            usleep(5000000);
         } while(success == false);

         rspDeleteSession(Session);
         Session = NULL;

         usleep(2000000);
      }
   }
}


#include "fractalpooluser.moc"


int main(int argc, char** argv)
{
   int n;
   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n],"-log",4))) {
         if(initLogging(argv[n]) == false) {
            exit(1);
         }
      }
   }
   beginLogging();
   if(rspInitialize(NULL) != 0) {
      puts("ERROR: Unable to initialize rsplib!");
      exit(1);
   }
   for(n = 1;n < argc;n++) {
      if(!(strncmp(argv[n], "-nameserver=" ,12))) {
         if(rspAddStaticNameServer((char*)&argv[n][12]) != RSPERR_OKAY) {
            fprintf(stderr, "ERROR: Bad name server setting: %s\n", argv[n]);
            exit(1);
         }
      }
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
