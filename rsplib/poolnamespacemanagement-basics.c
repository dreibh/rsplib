/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#include "poolnamespacemanagement-basics.h"
#include "stringutilities.h"


PoolElementSeqNumberType SeqNumberStart = (~0) ^ 0xf;


static const char* errorDescriptionArray[] =
{
   "no error",
   "no resources",
   "object not found",
   "invalid ID",
   "duplicate ID",
   "incompatible transport protocol",
   "incompatible control channel handling",
   "incompatible pool policy",
   "invalid pool policy",
   "invalid pool handle (too long)"
};


/* ###### Get error code description ##################################### */
const char* poolNamespaceManagementGetErrorDescription(const unsigned int errorCode)
{
   if(errorCode <= _PENC_LAST_CODE) {
      return(errorDescriptionArray[errorCode]);
   }
   return("(invalid error code)");
}


/* ###### Get textual description ######################################## */
void poolHandleGetDescription(const unsigned char* poolHandle,
                              const size_t         poolHandleSize,
                              char*                buffer,
                              const size_t         bufferSize)
{
   char   result[8];
   size_t i;

   buffer[0] = 0x00;
   for(i = 0;i < ((poolHandleSize <= MAX_POOLHANDLESIZE) ? poolHandleSize : MAX_POOLHANDLESIZE);i++) {
      if(!iscntrl(poolHandle[i])) {
         result[0] = poolHandle[i];
         result[1] = 0x00;
         safestrcat(buffer, result, bufferSize);
      }
      else {
         snprintf((char*)&result, sizeof(result), "{%02x}", poolHandle[i]);
         safestrcat(buffer, result, bufferSize);
      }
   }
}


/* ###### Print pool handle ############################################## */
void poolHandlePrint(const unsigned char* poolHandle,
                     const size_t         poolHandleSize,
                     FILE*                fd)
{
   char poolHandleDescription[1024];
   poolHandleGetDescription(poolHandle, poolHandleSize,
                            poolHandleDescription, sizeof(poolHandleDescription));
   fputs(poolHandleDescription, fd);
}


/* ###### Pool Handle comparison ######################################### */
int poolHandleComparison(const unsigned char* poolHandle1,
                         const size_t         poolHandleSize1,
                         const unsigned char* poolHandle2,
                         const size_t         poolHandleSize2)
{
   const int cmpResult = memcmp(poolHandle1, poolHandle2,
                                (poolHandleSize1 < poolHandleSize2) ? poolHandleSize1 : poolHandleSize2);
   if(cmpResult == 0) {
      if(poolHandleSize1 < poolHandleSize2) {
         return(-1);
      }
      if(poolHandleSize1 > poolHandleSize2) {
         return(1);
      }
      return(0);
   }
   return(cmpResult);
}


/* ###### Get 64-bit random number ####################################### */
unsigned long long random64()
{
   unsigned int       a      = random();
   unsigned int       b      = random();
   unsigned long long result = (((unsigned long long)a) << 32) | b;
   return(result);
}


/* ###### Get pool element identifier #################################### */
PoolElementIdentifierType getPoolElementIdentifier()
{
   PoolElementIdentifierType poolElementIdentifier = 1 + (random() % 0xfffffffe);
   return(poolElementIdentifier);
}
