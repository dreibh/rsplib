/*
 *  $Id: poolhandle.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Pool Handle
 *
 */


#include "tdtypes.h"
#include "poolhandle.h"



/* ###### Constructor #################################################### */
struct PoolHandle* poolHandleNew(const char* handle, const size_t length)
{
   struct PoolHandle* poolHandle = (struct PoolHandle*)malloc(sizeof(struct PoolHandle) + length);
   if(poolHandle != NULL) {
      poolHandle->Length = length;
      memcpy((char*)&poolHandle->Handle,handle,length);
   }
   return(poolHandle);
}


/* ###### Constructor #################################################### */
struct PoolHandle* poolHandleNewASCII(const char* poolName)
{
   return(poolHandleNew(poolName,strlen(poolName)));
}


/* ###### Destructor ##################################################### */
void poolHandleDelete(struct PoolHandle* poolHandle)
{
   if(poolHandle != NULL) {
      free(poolHandle);
   }
}


/* ###### Duplicate PoolHandle ########################################### */
struct PoolHandle* poolHandleDuplicate(const struct PoolHandle* source)
{
   struct PoolHandle* copy = NULL;
   if(source != NULL) {
      copy = poolHandleNew(source->Handle,source->Length);
   }
   return(copy);
}


/* ###### Print PoolHandle ############################################### */
void poolHandlePrint(const struct PoolHandle* poolHandle, FILE* fd)
{
   size_t i;

   if(poolHandle != NULL) {
      fputc('\"',fd);
      for(i = 0;i < poolHandle->Length;i++) {
         if(isprint(poolHandle->Handle[i])) {
            fputc(poolHandle->Handle[i],fd);
         }
         else {
            fprintf(fd,"{%02x}",(unsigned char)poolHandle->Handle[i]);
         }
      }
      fputc('\"',fd);
   }
   else {
      fprintf(fd,"(null)");
   }
}


/* ###### PoolHandle hash function ####################################### */
guint poolHandleHashFunc(gconstpointer key)
{
   const struct PoolHandle* poolHandle = (const struct PoolHandle*)key;
   guint                    hash = 0;
   size_t                   i;

   if(poolHandle->Length > 0) {
      hash = poolHandle->Handle[0];
      for(i = 1;i < poolHandle->Length;i++) {
         hash = (hash << 5) - hash + poolHandle->Handle[i];
      }
   }

   return(hash);
}


/* ###### PoolHandle compare function #################################### */
gint poolHandleCompareFunc(gconstpointer a,
                           gconstpointer b)
{
   const struct PoolHandle* h1 = (const struct PoolHandle*)a;
   const struct PoolHandle* h2 = (const struct PoolHandle*)b;

   const gint result = memcmp(&h1->Handle,&h2->Handle,min(h1->Length,h2->Length));
   if(result == 0) {
      if(h1->Length < h2->Length) {
         return(-1);
      }
      else if(h1->Length > h2->Length) {
         return(1);
      }
   }
   return(result);
}


/* ###### PoolHandle equality function ################################### */
gint poolHandleEqualFunc(gconstpointer a,
                         gconstpointer b)
{
   const struct PoolHandle* h1 = (const struct PoolHandle*)a;
   const struct PoolHandle* h2 = (const struct PoolHandle*)b;

   return(memcmp(&h1->Handle,&h2->Handle,min(h1->Length,h2->Length)) == 0);
}
