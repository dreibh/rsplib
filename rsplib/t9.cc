#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <math.h>
#include <stdint.h>
#include "netutilities.h"

void print(void* x)
{
   unsigned char* y = (unsigned char*)x;
   for(int i=0;i<8;i++) {
      printf("%02x ", y[i]);
   }
   puts("");
}




#define VERIFY

#ifndef CHECK
#define CHECK(cond) if(!(cond)) { fprintf(stderr, "INTERNAL ERROR in %s, line %u: condition %s is not satisfied!\n", __FILE__, __LINE__, #cond); abort(); }
#endif



#define DBL_EXP_BITS  11
#define DBL_EXP_BIAS  1023
#define DBL_EXP_MAX   ((1L << DBL_EXP_BITS) - 1 - DBL_EXP_BIAS)
#define DBL_EXP_MIN   (1 - DBL_EXP_BIAS)
#define DBL_FRC1_BITS 20
#define DBL_FRC2_BITS 32
#define DBL_FRC_BITS  (DBL_FRC1_BITS + DBL_FRC2_BITS)


typedef unsigned long long network_double_t;


struct IeeeDouble {
#if __BYTE_ORDER == __BIG_ENDIAN
   unsigned int s : 1;
   unsigned int e : 11;
   unsigned int f1 : 20;
   unsigned int f2 : 32;
#elif  __BYTE_ORDER == __LITTLE_ENDIAN
   unsigned int f2 : 32;
   unsigned int f1 : 20;
   unsigned int e : 11;
   unsigned int s : 1;
#else
#error Unknown byteorder settings!
#endif
};


/* ###### Convert double to machine-independent form ##################### */
network_double_t doubleToNetwork(const double d)
{
   IeeeDouble ieee;

   if(isnan(d)) {
      // NaN
      ieee.s = 0;
      ieee.e = DBL_EXP_MAX + DBL_EXP_BIAS;
      ieee.f1 = 1;
      ieee.f2 = 1;
   } else if(isinf(d)) {
      // +/- infinity
      ieee.s = (d < 0);
      ieee.e = DBL_EXP_MAX + DBL_EXP_BIAS;
      ieee.f1 = 0;
      ieee.f2 = 0;
   } else if(d == 0.0) {
      // zero
      ieee.s = 0;
      ieee.e = 0;
      ieee.f1 = 0;
      ieee.f2 = 0;
   } else {
      // finite number
      int exp;
      double frac = frexp (fabs (d), &exp);

      while (frac < 1.0 && exp >= DBL_EXP_MIN) {
         frac = ldexp (frac, 1);
         --exp;
      }
      if (exp < DBL_EXP_MIN) {
          // denormalized number (or zero)
          frac = ldexp (frac, exp - DBL_EXP_MIN);
          exp = 0;
      } else {
         // normalized number
         CHECK((1.0 <= frac) && (frac < 2.0));
         CHECK((DBL_EXP_MIN <= exp) && (exp <= DBL_EXP_MAX));

         exp += DBL_EXP_BIAS;
         frac -= 1.0;
      }
printf("frac=%f\n",frac);
printf("dbl=%d  %d\n",DBL_FRC1_BITS, DBL_FRC_BITS);
printf("f2=%1.9f\n",(double)ldexp(frac, DBL_FRC_BITS));
printf("f2=%u\n",(unsigned int)ldexp(frac, DBL_FRC_BITS));
      ieee.s = (d < 0);
      ieee.e = exp;
      ieee.f1 = (unsigned long)ldexp (frac, DBL_FRC1_BITS);
      ieee.f2 = (unsigned long)ldexp (frac, DBL_FRC_BITS);
   }
   return(hton64(*((network_double_t*)&ieee)));
}


/* ###### Convert machine-independent form to double ##################### */
double networkToDouble(network_double_t value)
{
   network_double_t hValue;
   IeeeDouble*      ieee;
   double           d;

   hValue = ntoh64(value);
   ieee = (IeeeDouble*)&hValue;
   if(ieee->e == 0) {
      if((ieee->f1 == 0) && (ieee->f2 == 0)) {
         // zero
         d = 0.0;
      } else {
         // denormalized number
         d  = ldexp((double)ieee->f1, -DBL_FRC1_BITS + DBL_EXP_MIN);
         d += ldexp((double)ieee->f2, -DBL_FRC_BITS  + DBL_EXP_MIN);
         if (ieee->s) {
            d = -d;
         }
      }
   } else if(ieee->e == DBL_EXP_MAX + DBL_EXP_BIAS) {
      if((ieee->f1 == 0) && (ieee->f2 == 0)) {
         // +/- infinity
         d = (ieee->s) ? -INFINITY : INFINITY;
      } else {
         // not a number
         d = NAN;
      }
   } else {
      // normalized number
      d = ldexp(ldexp((double)ieee->f1, -DBL_FRC1_BITS) +
                ldexp((double)ieee->f2, -DBL_FRC_BITS) + 1.0,
                      ieee->e - DBL_EXP_BIAS);
      if(ieee->s) {
         d = -d;
      }
   }
   return(d);
}


int main(int argc, char** argv)
{
   network_double_t n1 = doubleToNetwork(M_PI);

   printf("n1 = ");
   print((void*)&n1);

   puts("---");
   double w = networkToDouble(n1);
   printf("w=%1.20f\n",w);
   printf("o=%1.20f\n",M_PI);


#if 0
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
#endif
   return 0;
}
