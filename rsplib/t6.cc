#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include "randomizer.h"
#include "statistics.h"


int main(int argc, char** argv)
{
   Statistics stat;

   for(size_t i = 0;i < 1000000;i++) {
      double d = 999.0 + (random() % 3); // randomExpDouble(1000.0);

      stat.collect(d);

      // printf("%u %1.5f\n",i,d);
   }

   printf("mean=%f min=%f max=%f stddev=%f\n", stat.mean(), stat.minimum(),stat.maximum(),stat.stddev());
}
