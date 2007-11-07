#include "tcplikeserver.h"
#include "netutilities.h"
#include "netdouble.h"
#include "fractalgeneratorpackets.h"
#include "timeutilities.h"
#include "stringutilities.h"

#include <complex>


// #ifdef __LINUX__LINUX
#warning GNU!
#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
// #endif


class FractalGeneratorService
{
   public:
   struct FractalGeneratorServiceSettings
   {
      unsigned long long CookieMaxTime;
      size_t             CookieMaxPackets;
      size_t             TransmitTimeout;
      size_t             FailureAfter;
      bool               TestMode;
   };

   FractalGeneratorService();
   ~FractalGeneratorService();
   EventHandlingResult calculateImage();

   inline void setLoad(double load) { }
   bool sendData();
   inline bool sendCookie() { return(true); }
   inline bool isShuttingDown() { return(false); }

   private:
   EventHandlingResult advanceX(const unsigned int color);
   EventHandlingResult advanceY();


   unsigned long long             LastCookieTimeStamp;
   unsigned long long             LastSendTimeStamp;
   int                            RSerPoolSocketDescriptor;
   size_t                         DataPacketsAfterLastCookie;

   struct FGPData       Data;
   size_t               DataPackets;

   FractalGeneratorStatus         Status;
   FractalGeneratorServiceSettings Settings;
};


FractalGeneratorService::FractalGeneratorService()
{
   Settings.TestMode = false;
   Settings.FailureAfter = 0;
   Settings.CookieMaxTime = 1000000;
   Settings.CookieMaxPackets = 1000000;
   RSerPoolSocketDescriptor = -1;
   Status.Parameter.Width         = 1024;
   Status.Parameter.Height        = 768;
   Status.Parameter.C1Real        = doubleToNetwork(-2.0);
   Status.Parameter.C2Real        = doubleToNetwork(2.0);
   Status.Parameter.C1Imag        = doubleToNetwork(-2.0);
   Status.Parameter.C2Imag        = doubleToNetwork(2.0);
   Status.Parameter.N             = 2;
   Status.Parameter.MaxIterations = 100;
   Status.Parameter.AlgorithmID   = FGPA_MANDELBROT;
   Status.CurrentX                = 0;
   Status.CurrentY                = 0;
}


FractalGeneratorService::~FractalGeneratorService()
{
}


bool FractalGeneratorService::sendData()
{
   Data.Points = 0;
   Data.StartX = 0;
   Data.StartY = 0;
   LastSendTimeStamp = getMicroTime();
   DataPacketsAfterLastCookie++;
   return(true);
}


// ###### Set point and advance x position ##################################
EventHandlingResult FractalGeneratorService::advanceX(const unsigned int color)
{
   if(Data.Points == 0) {
      Data.StartX = Status.CurrentX;
      Data.StartY = Status.CurrentY;
   }
   Data.Buffer[Data.Points] = color;
   Data.Points++;

   // ====== Send Data ================================================
   if(unlikely( (Data.Points >= FGD_MAX_POINTS) ||
                ( (Data.Points & 0x3f) == 0x00) &&   /* getMicroTime() is expensive! */
                  (LastSendTimeStamp + 1000000 < getMicroTime()) )) {
      if(!sendData()) {
         printTimeStamp(stdout);
         printf("Sending Data on RSerPool socket %u failed!\n",
                RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }

      DataPackets++;
      if((Settings.FailureAfter > 0) && (DataPackets >= Settings.FailureAfter)) {
         sendCookie();
         printTimeStamp(stdout);
         printf("Failure Tester on RSerPool socket %u -> Disconnecting after %u packets!\n",
                RSerPoolSocketDescriptor,
                (unsigned int)DataPackets);
         return(EHR_Abort);
      }
   }

   Status.CurrentX++;
   return(EHR_Okay);
}


// ###### Advance y position ################################################
EventHandlingResult FractalGeneratorService::advanceY()
{
   Status.CurrentX = 0;
   Status.CurrentY++;

   // ====== Send cookie =================================================
   if( (DataPacketsAfterLastCookie >= Settings.CookieMaxPackets) ||
       (LastCookieTimeStamp + Settings.CookieMaxTime < getMicroTime()) ) {
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
   return(EHR_Okay);
}


// ###### Calculate image ####################################################
EventHandlingResult FractalGeneratorService::calculateImage()
{
   // ====== Update load ====================================================
//    setLoad(1.0 / ServerList->getMaxThreads());

   // ====== Initialize =====================================================
   EventHandlingResult  result = EHR_Okay;
   double               stepX;
   double               stepY;
   std::complex<double> z;
   size_t               i;
   const unsigned int   algorithm = (Settings.TestMode) ? 0 : Status.Parameter.AlgorithmID;

   DataPackets = 0;
   Data.Points = 0;
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


size_t u=0;
   // ====== Algorithms =====================================================
   switch(algorithm) {

      // ====== Mandelbrot ==================================================
      case FGPA_MANDELBROT:
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
               // ====== Calculate pixel ====================================
               const std::complex<double> c =
                  std::complex<double>(c1real + ((double)Status.CurrentX * stepX),
                                       c1imag + ((double)Status.CurrentY * stepY));

               z = std::complex<double>(0.0, 0.0);
               for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                  z = z*z - c;
u++;
                  if(unlikely(z.real() * z.real() + z.imag() * z.imag() >= 2.0)) {
                     break;
                  }
               }

               result = advanceX(i);
               if(unlikely(result != EHR_Okay)) {
                  break;
               }
            }
            result = advanceY();
            if(unlikely(result != EHR_Okay)) {
               break;
            }
         }
        break;

      // ====== MandelbrotN =================================================
      case FGPA_MANDELBROTN:
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
               // ====== Calculate pixel ====================================
               const std::complex<double> c =
                  std::complex<double>(c1real + ((double)Status.CurrentX * stepX),
                                       c1imag + ((double)Status.CurrentY * stepY));

               z = std::complex<double>(0.0, 0.0);
               for(i = 0;i < Status.Parameter.MaxIterations;i++) {
                  z = pow(z, (int)rint(n)) - c;
u++;
                  if(unlikely(z.real() * z.real() + z.imag() * z.imag()) >= 2.0) {
                     break;
                  }
               }

               result = advanceX(i);
               if(unlikely(result != EHR_Okay)) {
                  break;
               }
            }
            result = advanceY();
            if(unlikely(result != EHR_Okay)) {
               break;
            }
         }
        break;

      // ====== Test Mode ===================================================
      default:
         while(Status.CurrentY < Status.Parameter.Height) {
            while(Status.CurrentX < Status.Parameter.Width) {
u++;
               result = advanceX((Status.CurrentX * Status.CurrentY) % 256);
               if(unlikely(result != EHR_Okay)) {
                  break;
               }
            }
            result = advanceY();
            if(unlikely(result != EHR_Okay)) {
               break;
            }
         }
        break;

   }
printf("U=%d\n", u);

   // ====== Send last Data packet ==========================================
   if(Data.Points > 0) {
      if(!sendData()) {
         printf("Sending Data (last block) on RSerPool socket %u failed!\n",
                RSerPoolSocketDescriptor);
         return(EHR_Abort);
      }
   }

   Data.StartX = 0xffffffff;
   Data.StartY = 0xffffffff;
   if(!sendData()) {
      printf("Sending Data (end of Data) on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }

   setLoad(0.0);
   return(EHR_Okay);
}



int main(int argc, char** argv)
{
   FractalGeneratorService fgs;
   unsigned long long t0 = getMicroTime();
   fgs.calculateImage();
   unsigned long long t1 = getMicroTime();
   printf("T=%1.6fs\n", (t1 - t0) / 1000000.0);
}
