/*
 *  $Id: poolelementhandle.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Pool Element Handle
 *
 */


#include "tdtypes.h"
#include "poolelementhandle.h"


/*
   ????  Thread-safety required!
*/


/* ###### Get pool element handle ######################################## */
PoolElementHandle getPoolElementHandle()
{
   static PoolElementHandle Handle = UNDEFINED_POOL_ELEMENT_HANDLE;
   return(++Handle);
}


/* ###### Pool element handle hash function ############################## */
guint poolElementHandleHashFunc(gconstpointer key)
{
   const PoolElementHandle handle = *((const PoolElementHandle*)key);

   /* 4294967291 = 2^32-5 is a prime number */
   guint hash = (guint)(handle % (uint64_t)4294967291U);
   return(hash);
}


/* ###### Pool element handle comparision function ####################### */
gint poolElementHandleCompareFunc(gconstpointer a,
                                  gconstpointer b)
{
   const PoolElementHandle h1 = *((const PoolElementHandle*)a);
   const PoolElementHandle h2 = *((const PoolElementHandle*)b);

   if(h1 < h2) {
      return(-1);
   }
   else if(h1 > h2) {
      return(1);
   }
   return(0);
}


/* ###### Pool element handle equality function ########################## */
gint poolElementHandleEqualFunc(gconstpointer a,
                                gconstpointer b)
{
   const PoolElementHandle h1 = *((const PoolElementHandle*)a);
   const PoolElementHandle h2 = *((const PoolElementHandle*)b);

   return(h1 == h2);
}
