/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "netdouble.h"
#include "netutilities.h"
#include "debug.h"


#ifndef HAVE_IEEE_FP
#warning Is this code really working correctly?

#define DBL_EXP_BITS  11
#define DBL_EXP_BIAS  1023
#define DBL_EXP_MAX   ((1L << DBL_EXP_BITS) - 1 - DBL_EXP_BIAS)
#define DBL_EXP_MIN   (1 - DBL_EXP_BIAS)
#define DBL_FRC1_BITS 20
#define DBL_FRC2_BITS 32
#define DBL_FRC_BITS  (DBL_FRC1_BITS + DBL_FRC2_BITS)


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
   struct IeeeDouble ieee;

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
   network_double_t   hValue;
   struct IeeeDouble* ieee;
   double             d;

   hValue = ntoh64(value);
   ieee = (struct IeeeDouble*)&hValue;
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

#else

/* ###### Convert double to machine-independent form ##################### */
network_double_t doubleToNetwork(const double d)
{
   const unsigned long long* v = (const unsigned long long*)&d;
   return(hton64(*v));
}

/* ###### Convert machine-independent form to double ##################### */
double networkToDouble(network_double_t value)
{
   unsigned long long w = ntoh64(value);
   return(*((double*)&w));
}

#endif
