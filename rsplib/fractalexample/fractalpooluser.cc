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

   inline size_t width() const {
      return(Width);
   }

   inline size_t height() const {
      return(Height);
   }

   protected:
   void paintImage(const size_t startY, const size_t endY);
   void resizeEvent(QResizeEvent* resizeEvent);
   void paintEvent(QPaintEvent* paintEvent);

   private:
   virtual void run();

   bool                 Running;
   size_t               Width;
   size_t               Height;
   QImage*              Image;

   unsigned char*       PoolHandle;
   size_t               PoolHandleSize;
   SessionDescriptor*   Session;
   std::complex<double> C1;
   std::complex<double> C2;
   size_t               MaxIterations;
   unsigned int         AlgorithmID;
};


FractalPU::FractalPU(const size_t width,
                     const size_t height,
                     QWidget*     parent,
                     const char*  name)
   : QWidget(parent, name)
{
   Running  = true;
   Width    = width;
   Height   = height;
   resize(width, height);
   Image  = new QImage(Width, Height, 32);
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
               Width, endY + 1);
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


void FractalPU::run()
{
   size_t  run = 0;
   TagItem tags[16];

   for(size_t x = 0;x < Width;x++) {
      for(size_t y = 0;y < Height;y++) {
         setPoint(x, y, qRgb(16* run, x % 256, y % 256));
      }
   }
   update();

   C1            = std::complex<double>(-1.5,1.5);
   C2            = std::complex<double>(1.5,-1.5);
   MaxIterations = 150;
   AlgorithmID   = FGPA_MANDELBROT;

   PoolHandle     = (unsigned char*)"FractalGeneratorPool";
   PoolHandleSize = strlen((const char*)PoolHandle);
   tags[0].Tag = TAG_TuneSCTP_MinRTO;
   tags[0].Data = 100;
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
   Session = rspCreateSession(PoolHandle, PoolHandleSize, NULL, (TagItem*)&tags);
   if(Session) {
      for(;;) {
         puts("Sending parameter command...");
         run++;

         bool success = false;
         do {
            FractalGeneratorParameter parameter;
            parameter.Width         = htonl(Width);
            parameter.Height        = htonl(Height);
            parameter.AlgorithmID   = htonl(AlgorithmID);
            parameter.MaxIterations = htonl(MaxIterations);
            parameter.C1Real        = C1.real();
            parameter.C1Imag        = C1.imag();
            parameter.C2Real        = C2.real();
            parameter.C2Imag        = C2.imag();

            tags[0].Tag  = TAG_RspIO_Timeout;
            tags[0].Data = (tagdata_t)2000000;
            tags[1].Tag  = TAG_DONE;
            if(rspSessionWrite(Session, &parameter, sizeof(parameter),
                              (TagItem*)&tags) <= 0) {
               puts("ERROR: SessionWrite failed!");
            }
            else {
               FractalGeneratorData data;

               tags[0].Tag  = TAG_RspIO_Timeout;
               tags[0].Data = (tagdata_t)2000000;
               tags[1].Tag  = TAG_DONE;
               ssize_t received = rspSessionRead(Session, &data, sizeof(data), (TagItem*)&tags);
               for(;;) {
                  if( (received >= (ssize_t)getFractalGeneratorDataSize(0)) &&
                      (received >= (ssize_t)getFractalGeneratorDataSize(data.Points)) ) {
                     if(data.Points == 0) {
                        puts("ENDE!");
                        success = true;
                        break;
                     }
                     size_t p = 0;
                     size_t x = data.StartX;
                     size_t y = data.StartY;
                     while(y < Height) {
                        while(x < Width) {
                           if(p > data.Points) {
                              break;
                           }

                           const QColor color(((data.Buffer[p] + run) % 72) * 5, 255, 255, QColor::Hsv);
                           setPoint(x, y, color.rgb());
                           p++;

                           x++;
                        }
                        x = 0;
                        y++;
                     }
                     paintImage(data.StartY, y);
                  }
                  else {
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

         usleep(2000000);
      }
      rspDeleteSession(Session);
      Session = NULL;
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
