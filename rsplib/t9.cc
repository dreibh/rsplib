#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <math.h>
#include <stdint.h>
#include "netutilities.h"
#include "netdouble.h"


void print(const void* x)
{
   unsigned char* y = (unsigned char*)x;
   for(int i=0;i<8;i++) {
      printf("%02x ", y[i]);
   }
   puts("");
}





int main(int argc, char** argv)
{

  double value;

  value = 0.0;
  if(*(int *)&value != 0) {
    return(1);
  }

  value = 1000.5;
  if(*(unsigned long long *)&value != 0x408f440000000000ULL) {
    return(1);
  }

  value = -1000.5;
  if(*(unsigned long long *)&value != 0xc08f440000000000ULL) {
    return(1);
  }

  return(0);


/*
   network_double_t n1 = doubleToNetwork(M_PI);

   printf("n1 = ");
   print((void*)&n1);

   puts("---");
   double w = networkToDouble(n1);
   printf("w=%1.20f\n",w);
   printf("o=%1.20f\n",M_PI);
   printf("Hex=");print((void*)&n1);
*/
/*
   network_double_t v[12];
   network_double_t w[sizeof(v) / sizeof(network_double_t)];
   FILE* fh;

   printf("sizeof(network_double_t) = %u\n", sizeof(network_double_t));
   CHECK(sizeof(network_double_t) == 8);

   v[0] = doubleToNetwork(M_PI);
   v[1] = doubleToNetwork(-M_PI);
   v[2] = doubleToNetwork(1000000.0);
   v[3] = doubleToNetwork(-1000000.0);
   v[4] = doubleToNetwork(1.0 / 8.0);
   v[5] = doubleToNetwork(-1.0 / 8.0);
   v[6] = doubleToNetwork(32.0);
   v[7] = doubleToNetwork(-1.0/65536.0);
   v[8] = doubleToNetwork(-INFINITY);
   v[9] = doubleToNetwork(+INFINITY);
   v[10] = doubleToNetwork(NAN);
   v[11] = doubleToNetwork(2000000000.0 * 2000000000.0 * 1234.0);


//    fh = fopen("test.dat", "w");
//    if(fh == NULL) {
//       perror("fopen()");
//       exit(1);
//    }
//    if(fwrite((void*)&v, sizeof(v), 1, fh) != 1) {
//       perror("fwrite()");
//       exit(1);
//    }
//    fclose(fh);
//    for(size_t i = 0;i < sizeof(w) / sizeof(network_double_t);i++) {
//       printf("v[%u]=%1.20f\n", i, networkToDouble(v[i]));
//    }


   puts("----");


   fh = fopen("test.dat", "r");
   if(fread((void*)&w, sizeof(w), 1, fh) != 1) {
      perror("fwrite()");
      exit(1);
   }
   fclose(fh);
   for(size_t i = 0;i < sizeof(w) / sizeof(network_double_t);i++) {
      printf("w[%u]=%1.20f\n", i, networkToDouble(w[i]));
      //CHECK(fabs(v[i]-w[i]) < 0.001);
   }


   for(size_t i = 0;i < 100000;i++) {
      double vx = (double)random() * (double)random();
      double vy = (double)random() / (double)random();
      // printf("%1.9f   %1.9f\n",vx,vy);
      network_double_t t1 = doubleToNetwork(vx);
      network_double_t t2 = doubleToNetwork(vy);
      network_double_t t3 = doubleToNetwork(-vx);
      network_double_t t4 = doubleToNetwork(-vy);
   }
*/
   return 0;
}
