#include "timeutilities.h"
#include "netutilities.h"
#include "threadsafety.h"
#include "breakdetector.h"
#include "asapinstance.h"
#include "rsputilities.h"
#include "rserpool.h"
#include "loglevel.h"


extern struct ASAPInstance* gAsapInstance;


int main(int argc, char** argv)
{
   struct rsp_info info;

   memset(&info, 0, sizeof(info));
   for(int i = 1;i < argc;i++) {
      if(!(strncmp(argv[i], "-log" ,4))) {
         if(initLogging(argv[i]) == false) {
            exit(1);
         }
      }
      else if(!(strncmp(argv[i], "-registrar=", 11))) {
         if(addStaticRegistrar(&info, (char*)&argv[i][11]) < 0) {
            fprintf(stderr, "ERROR: Bad registrar setting %s\n", argv[i]);
            exit(1);
         }
      }
   }
   beginLogging();

   if(rsp_initialize(&info) < 0) exit(1);

   puts("START!");

   // installBreakDetector();
   int i = 0x1000000;
   while(!breakDetected()) {
      usleep(1000000);

      struct PoolHandle ph;
      poolHandleNew(&ph, (unsigned char*)"TestPool", 8);

      struct ST_CLASS(PoolElementNode) pe;
      PoolPolicySettings poolPolicySettings;
      poolPolicySettings.PolicyType      = PPT_LEASTUSED;
      poolPolicySettings.Weight          = 0;
      poolPolicySettings.Load            = 1;
      poolPolicySettings.LoadDegradation = 0;
      sockaddr_union a;
      string2address("127.0.0.1", &a);
      char tbbuf[1000];
      TransportAddressBlock* tb = (TransportAddressBlock*)&tbbuf;
      transportAddressBlockNew(tb, IPPROTO_TCP, 1234, 0, &a, 1);
      ST_CLASS(poolElementNodeNew)(&pe, 0xffffffff-i, 0, 100000, &poolPolicySettings,
                                   tb, NULL, -1, 0);

/*
      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), false));
      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), true));
      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), false));
      printf("REG.RESULT=$%x\n",asapInstanceRegister(gAsapInstance, &ph, &pe, true));
      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      puts("HRES...");
*/

//       printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
       printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));

      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), false));
      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), false));

      struct ST_CLASS(PoolElementNode)* poolElementNodeArray;
      size_t s;
      printf("HRES.RESULT=$%x\n",asapInstanceHandleResolution(gAsapInstance, &ph, &poolElementNodeArray, &s));

      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), false));
      printf("DEREG.RESULT=$%x\n",asapInstanceDeregister(gAsapInstance, &ph, 1 + (i++), false));

      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));
      printf("REPORT.RESULT=$%x\n",asapInstanceReportFailure(gAsapInstance, &ph, 0x12345678));

      puts("---------------------------------------");
   }

   rsp_cleanup();
   return(0);
}
