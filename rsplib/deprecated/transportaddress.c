/*
 *  $Id: transportaddress.c,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Transport Address
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "transportaddress.h"
#include "utilities.h"
#include "netutilities.h"

#include <ext_socket.h>



/* ###### Constructor #################################################### */
struct TransportAddress* transportAddressNew(const int                      protocol,
                                             const uint16_t                 port,
                                             const struct sockaddr_storage* addressArray,
                                             const size_t                   addresses)
{
   struct TransportAddress* transportAddress;
   size_t                   i;

   transportAddress = (struct TransportAddress*)malloc(sizeof(struct TransportAddress));
   if(transportAddress != NULL) {
      transportAddress->Protocol     = protocol;
      transportAddress->Addresses    = addresses;
      transportAddress->Port         = port;
      transportAddress->AddressArray = (union sockaddr_union*)malloc(sizeof(union sockaddr_union) * addresses);
      if(transportAddress->AddressArray != NULL) {
         for(i = 0;i < addresses;i++) {
            memcpy((void*)&transportAddress->AddressArray[i],
                   (void*)&addressArray[i],
                   sizeof(union sockaddr_union));
            switch(((struct sockaddr*)&addressArray[i])->sa_family) {
               case AF_INET:
                   ((struct sockaddr_in*)&transportAddress->AddressArray[i])->sin_port = htons(port);
                break;
               case AF_INET6:
                   ((struct sockaddr_in6*)&transportAddress->AddressArray[i])->sin6_port = htons(port);
                break;
               default:
                   LOG_ERROR
                   fprintf(stdlog,"Unsupported address family #%d\n",((struct sockaddr*)&addressArray[i])->sa_family);
                   LOG_END
                   transportAddressDelete(transportAddress);
                   transportAddress = NULL;
                break;
            }
         }
      }
      else {
         transportAddressDelete(transportAddress);
         transportAddress = NULL;
      }
   }

   return(transportAddress);
}


/* ###### Destructor ##################################################### */
void transportAddressDelete(struct TransportAddress* transportAddress)
{
   if(transportAddress != NULL) {
      if(transportAddress->AddressArray != NULL) {
         free(transportAddress->AddressArray);
      }
      free(transportAddress);
   }
}


/* ###### Duplicate TransportAddress ##################################### */
struct TransportAddress* transportAddressDuplicate(const struct TransportAddress* source)
{
   struct TransportAddress* copy = NULL;

   if(source != NULL) {
      copy = (struct TransportAddress*)malloc(sizeof(struct TransportAddress));
      if(copy != NULL) {
         copy->Protocol     = source->Protocol;
         copy->Port         = source->Port;
         copy->Addresses    = source->Addresses;
         copy->AddressArray = (union sockaddr_union*)memdup(source->AddressArray,
                                                            sizeof(union sockaddr_union) * source->Addresses);
         if(copy->AddressArray == NULL) {
            transportAddressDelete(copy);
            copy = NULL;
         }
      }
   }
   return(copy);
}


/* ###### Print TransportAddress ######################################### */
void transportAddressPrint(const struct TransportAddress* transportAddress, FILE* fd)
{
   char   str[64];
   size_t i;

   if(transportAddress != NULL) {
      fputs("{",fd);
      for(i = 0;i < transportAddress->Addresses;i++) {
         if(i > 0) {
            fputs(", ",fd);
         }
         if(address2string((struct sockaddr*)&transportAddress->AddressArray[i],
                           (char*)&str, sizeof(str), false)) {
            fprintf(fd," %s",str);
         }
         else {
            fputs("(invalid)",fd);
         }
      }
      fputs(" }",fd);
      switch(transportAddress->Protocol) {
         case IPPROTO_SCTP:
            strcpy((char*)&str,"SCTP");
          break;
         case IPPROTO_TCP:
            strcpy((char*)&str,"TCP");
          break;
         case IPPROTO_UDP:
            strcpy((char*)&str,"UDP");
          break;
         default:
            snprintf((char*)&str,sizeof(str),"Protocol $%04x",transportAddress->Protocol);
          break;
      }
      fprintf(fd," %d/%s",transportAddress->Port,str);
   }
   else {
      fputs("(null)",fd);
   }
}


/* ###### Print TransportAddress ######################################### */
void transportAddressScopeFilter(struct TransportAddress* transportAddress,
                                 const unsigned int       minScope)
{
   size_t i, j;

   if(transportAddress != NULL) {
      for(i = 0, j = 0;i < transportAddress->Addresses;i++) {
         if(getScope((struct sockaddr*)&transportAddress->AddressArray[j]) > minScope) {
            if(i != j) {
               memcpy((void*)&transportAddress->AddressArray[j],
                      (void*)&transportAddress->AddressArray[i],
                      sizeof(transportAddress->AddressArray[i]));
            }
            j++;
         }
      }
      transportAddress->Addresses = j;
   }
}


/* ###### TransportAddress comparision function ########################## */
gint transportAddressCompareFunc(gconstpointer a, gconstpointer b)
{
   const struct TransportAddress* t1 = (const struct TransportAddress*)a;
   const struct TransportAddress* t2 = (const struct TransportAddress*)b;
   int    result;
   size_t i;

   if(t1->Protocol < t2->Protocol) {
      return(-1);
   }
   else if(t1->Protocol > t2->Protocol) {
      return(1);
   }

   if(t1->Port < t2->Port) {
      return(-1);
   }
   else if(t1->Port > t2->Port) {
      return(1);
   }

   for(i = 0;i < (size_t)min(t1->Addresses, t2->Addresses);i++) {
      result = addresscmp((struct sockaddr*)&t1->AddressArray[i],
                          (struct sockaddr*)&t2->AddressArray[i],
                          true);
      if(result != 0) {
         return(result);
      }
   }

   if(t1->Addresses < t2->Addresses) {
      return(-1);
   }
   else if(t1->Addresses > t2->Addresses) {
      return(1);
   }
   return(0);
}


/* ###### Duplicate TransportAddress list ################################ */
GList* transportAddressListDuplicate(GList* source)
{
   GList*                   copy;
   GList*                   element;
   GList*                   transportAddressList;
   struct TransportAddress* transportAddress;

   copy = NULL;
   if(source != NULL) {
      transportAddressList = g_list_first(source);
      while(transportAddressList != NULL) {
         transportAddress = transportAddressDuplicate((struct TransportAddress*)transportAddressList->data);
         if(transportAddress == NULL) {
            while(copy != NULL) {
               element = g_list_first(copy);
               transportAddress = (struct TransportAddress*)element->data;
               copy = g_list_remove(copy,transportAddress);
               free(transportAddress);
            }
            return(NULL);
         }
         copy = g_list_append(copy,transportAddress);
         transportAddressList = g_list_next(transportAddressList);
      }
   }
   return(copy);
}


/* ###### Delete TransportAddress list ################################### */
void transportAddressListDelete(GList* transportAddressList)
{
   struct TransportAddress* transportAddress;
   GList*                   list;

   list = g_list_first(transportAddressList);
   while(list != NULL) {
      transportAddress     = (struct TransportAddress*)list->data;
      transportAddressList = g_list_remove(transportAddressList, transportAddress);
      transportAddressDelete(transportAddress);
      list = g_list_first(transportAddressList);
   }
}


/* ###### TransportAddress list comparision function ###################### */
gint transportAddressListCompareFunc(gconstpointer a, gconstpointer b)
{
   GList* t1 = g_list_first((GList*)a);
   GList* t2 = g_list_first((GList*)b);
   gint   result;

   while((t1 != NULL) && (t2 != NULL)) {
      result = transportAddressCompareFunc(t1->data,t2->data);
      if(result != 0) {
         return(result);
      }
      t1 = g_list_next(t1);
      t2 = g_list_next(t2);
   }

   if((t1 == NULL) && (t2 != NULL)) {
      return(-1);
   }
   else if((t2 == NULL) && (t1 != NULL)) {
      return(1);
   }
   return(0);
}
