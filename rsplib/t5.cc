#include "tcplikeserver.h"
#include "netutilities.h"
#include "netdouble.h"
#include "fractalgeneratorpackets.h"
#include "timeutilities.h"
#include "stringutilities.h"

#include <complex>


#if defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 303)
#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#warning xy
#define likely(x)   (x)
#define unlikely(x) (x)
#endif


unsigned int gmt=0;
inline unsigned long long getMicroTime2() { gmt++; return(::getMicroTime()); }
#define getMicroTime()  getMicroTime2()


class FractalGeneratorServer
{
   public:
   struct FractalGeneratorServerSettings
   {
      unsigned long long CookieMaxTime;
      size_t             CookieMaxPackets;
      size_t             TransmitTimeout;
      size_t             FailureAfter;
      bool               TestMode;
   };

   FractalGeneratorServer();
   ~FractalGeneratorServer();
   EventHandlingResult calculateImage();

   inline void setLoad(double load) { }
   inline bool isShuttingDown() { return(false); }

   private:
   bool sendCookie();
   bool sendData();
   EventHandlingResult advanceX(const unsigned int color);
   EventHandlingResult advanceY();


   unsigned long long             LastCookieTimeStamp;
//  ???    unsigned long long             LastSendTimeStamp;
   int                            RSerPoolSocketDescriptor;
   size_t                         DataPacketsAfterLastCookie;

   struct FGPData       Data;
   size_t               DataPackets;

   FractalGeneratorStatus         Status;
   FractalGeneratorServerSettings Settings;
};


FractalGeneratorServer::FractalGeneratorServer()
{
   Settings.TestMode = false;
   Settings.FailureAfter = 0;
   Settings.CookieMaxTime = 1000000;
   Settings.CookieMaxPackets = 1000000;
   RSerPoolSocketDescriptor = -1;
   Status.Parameter.Width         = 20*1024;
   Status.Parameter.Height        = 20*768;
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


FractalGeneratorServer::~FractalGeneratorServer()
{
}


// ###### Send cookie #######################################################
bool FractalGeneratorServer::sendCookie()
{
   FGPCookie cookie;
   ssize_t   sent;

   strncpy((char*)&cookie.ID, FGP_COOKIE_ID, sizeof(cookie.ID));
   cookie.Parameter.Header.Type   = FGPT_PARAMETER;
   cookie.Parameter.Header.Flags  = 0x00;
   cookie.Parameter.Header.Length = htons(sizeof(cookie.Parameter.Header));
   cookie.Parameter.Width         = htonl(Status.Parameter.Width);
   cookie.Parameter.Height        = htonl(Status.Parameter.Height);
   cookie.Parameter.MaxIterations = htonl(Status.Parameter.MaxIterations);
   cookie.Parameter.AlgorithmID   = htonl(Status.Parameter.AlgorithmID);
   cookie.Parameter.C1Real        = Status.Parameter.C1Real;
   cookie.Parameter.C2Real        = Status.Parameter.C2Real;
   cookie.Parameter.C1Imag        = Status.Parameter.C1Imag;
   cookie.Parameter.C2Imag        = Status.Parameter.C2Imag;
   cookie.Parameter.N             = Status.Parameter.N;
   cookie.CurrentX                = htonl(Status.CurrentX);
   cookie.CurrentY                = htonl(Status.CurrentY);

/*
   sent = rsp_send_cookie(RSerPoolSocketDescriptor,
                          (unsigned char*)&cookie, sizeof(cookie), 0,
                          Settings.TransmitTimeout);
*/
sent=sizeof(cookie); // ???????????????????

   LastCookieTimeStamp        = getMicroTime();
   DataPacketsAfterLastCookie = 0;

   if(unlikely(sent != sizeof(cookie))) {
      printTimeStamp(stdout);
      printf("Sending cookie on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(false);
   }
   return(true);
}


// ###### Send data #########################################################
bool FractalGeneratorServer::sendData()
{
   const size_t dataSize = getFGPDataSize(Data.Points);
   ssize_t      sent;

   Data.Header.Type   = FGPT_DATA;
   Data.Header.Flags  = 0x00;
   Data.Header.Length = htons(dataSize);
   Data.Points        = htonl(Data.Points);
   Data.StartX        = htonl(Data.StartX);
   Data.StartY        = htonl(Data.StartY);

/*
   sent = rsp_sendmsg(RSerPoolSocketDescriptor,
                      data, dataSize, 0,
                      0, htonl(PPID_FGP), 0, 0, 0,
                      Settings.TransmitTimeout);
*/
sent=dataSize; // ??????????????????????????????????????????????????????????

   DataPackets++;
   DataPacketsAfterLastCookie++;
   Data.Points        = 0;
   Data.StartX        = 0;
   Data.StartY        = 0;
//    LastSendTimeStamp  = getMicroTime();

   if(unlikely(sent != (ssize_t)dataSize)) {
      printTimeStamp(stdout);
      printf("Sending Data on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(false);
   }
   return(true);
}


// ###### Set point and advance x position ##################################
EventHandlingResult FractalGeneratorServer::advanceX(const unsigned int color)
{
   if(Data.Points == 0) {
      Data.StartX = Status.CurrentX;
      Data.StartY = Status.CurrentY;
   }
   Data.Buffer[Data.Points] = htonl(color);
   Data.Points++;

   // ====== Send Data ================================================
   if(unlikely(Data.Points >= FGD_MAX_POINTS)) {
      if(unlikely(!sendData())) {
         return(EHR_Abort);
      }

      if(unlikely((Settings.FailureAfter > 0) && (DataPackets >= Settings.FailureAfter))) {
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
EventHandlingResult FractalGeneratorServer::advanceY()
{
   Status.CurrentY++;
   Status.CurrentX = 0;
   const unsigned long long now = getMicroTime();

   // ====== Send Data ======================================================
//    if(unlikely(LastSendTimeStamp + 1000000 < now)) {
//       if(unlikely(!sendData())) {
//          return(EHR_Abort);
//       }
//    }

   // ====== Send cookie ====================================================
   if( (DataPacketsAfterLastCookie >= Settings.CookieMaxPackets) ||
       (LastCookieTimeStamp + Settings.CookieMaxTime < now) ) {
      if(unlikely(!sendCookie())) {
         return(EHR_Abort);
      }
   }

   // ====== Shutdown =======================================================
   if(unlikely(isShuttingDown())) {
      printf("Aborting session on RSerPool socket %u due to server shutdown!\n",
            RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }
   return(EHR_Okay);
}


// ###### Calculate image ####################################################
EventHandlingResult FractalGeneratorServer::calculateImage()
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
//    LastCookieTimeStamp = LastSendTimeStamp = getMicroTime();

   if(!sendCookie()) {
      printTimeStamp(stdout);
      printf("Sending cookie (start) on RSerPool socket %u failed!\n",
             RSerPoolSocketDescriptor);
      return(EHR_Abort);
   }


unsigned long long u=0;
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
                  if(unlikely(z.real() * z.real() + z.imag() * z.imag() >= 2.0)) {
                     break;
                  }
               }

               result = advanceX(i);
               if(unlikely(result != EHR_Okay)) {
                  break;
               }
            }
u++;
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
printf("U=%llu\n", u);
printf("GMT=%d\n", gmt);

   // ====== Send last Data packet ==========================================
   if(Data.Points > 0) {
      if(unlikely(!sendData())) {
         return(EHR_Abort);
      }
   }

   Data.StartX = 0xffffffff;
   Data.StartY = 0xffffffff;
   if(unlikely(!sendData())) {
      return(EHR_Abort);
   }

   setLoad(0.0);
   return(EHR_Okay);
}



int main(int argc, char** argv)
{
   FractalGeneratorServer fgs;
   unsigned long long t0 = getMicroTime();
   fgs.calculateImage();
   unsigned long long t1 = getMicroTime();
   printf("T=%1.6fs\n", (t1 - t0) / 1000000.0);
}
