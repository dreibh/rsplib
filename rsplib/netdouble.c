/*
 *  $Id: netdouble.c,v 1.2 2005/08/01 10:01:18 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Network-Transferable Double Datatype
 *
 */

#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "netdouble.h"


/* ###### Convert double to machine-independent form ##################### */
network_double_t doubleToNetwork(double value)
{
   const double     absoluteValue = fabs(value);
   network_double_t result;
   uint16_t         flags;
   int16_t          exponent;
   uint64_t         mantissa;

   flags = 0;
   if(value < 0.0) {
      flags |= NDF_NEGATIVE;
   }
   exponent = -(int16_t)ceil(log(absoluteValue) / log(2.0));
   mantissa = (uint64_t)rint(absoluteValue *
                             pow(2.0, (MANTISSA_BITS - 1) + exponent));

   result.Flags        = htons(flags);
   result.Exponent     = htons(exponent);
   result.MantissaHigh = htonl((uint32_t)((mantissa >> 32) & 0xffffffffUL));
   result.MantissaLow  = htonl((uint32_t)(mantissa & 0xffffffffUL));

/*
   double p = networkToDouble(result);
   printf("   v=%1.12f => s=%d; m=%Lu; e=%d   p=%1.12f",
          value,
          ((result.Flags & NDF_NEGATIVE) ? -1 : 1), mantissa, exponent,
          p);
*/

#ifdef VERIFY
   CHECK(fabs(networkToDouble(result) - value) < 0.000000001);
#endif
   return(result);
}


/* ###### Convert machine-independent form to double ##################### */
double networkToDouble(const network_double_t value)
{
   uint16_t  flags;
   int16_t   exponent;
   uint64_t  mantissa;

   flags    = ntohs(value.Flags);
   exponent = (int16_t)ntohs(value.Exponent);
   mantissa = ((uint64_t)ntohl(value.MantissaHigh) << 32) | (uint64_t)ntohl(value.MantissaLow);

   double result = ((flags & NDF_NEGATIVE) ? -1.0 : 1.0) *
                      (double)mantissa /
                         pow(2.0, (MANTISSA_BITS - 1) + exponent);
   return(result);
}
