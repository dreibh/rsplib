/*
 *  $Id: poolelement.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Pool Element
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "poolelement.h"
#include "utilities.h"
#include "pool.h"
#include "asaperror.h"



/* ###### Constructor #################################################### */
struct PoolElement* poolElementNew(const PoolElementIdentifier poolElementIdentifier,
                                   const struct PoolPolicy*    poolPolicy,
                                   const bool                  hasControlChannel)
{
   struct PoolElement* poolElement;

   poolElement = (struct PoolElement*)malloc(sizeof(struct PoolElement));
   if(poolElement != NULL) {
      poolElement->Identifier           = poolElementIdentifier;
      poolElement->RegistrationLife     = (uint32_t)-1;
      poolElement->HomeENRPServerID     = 0;
      poolElement->HasControlChannel    = hasControlChannel;
      poolElement->TransportAddresses   = 0;
      poolElement->TransportAddressList = NULL;
      poolElement->OwnerPool            = NULL;
      poolElement->UserData             = NULL;
      poolElement->Flags                = 0;
      poolElement->UserCounter          = 0;
      poolElement->TimeStamp            = getMicroTime();

      poolElement->Policy = poolPolicyDuplicate(poolPolicy);
      if((poolElement->Policy == NULL) && (poolPolicy != NULL)) {
         poolElementDelete(poolElement);
         poolElement = NULL;
      }
   }
   return(poolElement);
}


/* ###### Destructor ##################################################### */
void poolElementDelete(struct PoolElement* poolElement)
{
   if(poolElement != NULL) {
      if(poolElement->UserCounter > 0) {
         LOG_WARNING
         fputs("UserCounter > 0!\n",stdlog);
         LOG_END
      }
      if(poolElement->OwnerPool != NULL) {
         LOG_WARNING
         fputs("Pool element is still in pool!\n",stdlog);
         LOG_END
      }

      if(poolElement->TransportAddressList) {
         transportAddressListDelete(poolElement->TransportAddressList);
         poolElement->TransportAddressList = NULL;
      }
      if(poolElement->Policy) {
         poolPolicyDelete(poolElement->Policy);
         poolElement->Policy = NULL;
      }

      poolElement->OwnerPool = NULL;
      free(poolElement);
   }
}


/* ###### Duplicate PoolElement ########################################## */
struct PoolElement* poolElementDuplicate(const struct PoolElement* source)
{
   struct PoolElement* copy = NULL;

   if(source != NULL) {
      copy = poolElementNew(source->Identifier,
                            source->Policy,
                            source->HasControlChannel);
      if(copy != NULL) {
         copy->HomeENRPServerID  = source->HomeENRPServerID;
         copy->RegistrationLife  = source->RegistrationLife;
         copy->TimeStamp         = source->TimeStamp;

         copy->TransportAddressList = transportAddressListDuplicate(source->TransportAddressList);
         if((copy->TransportAddressList == NULL) && (source->TransportAddressList != NULL)) {
            poolElementDelete(copy);
            return(NULL);
         }
         copy->TransportAddresses = source->TransportAddresses;
      }
   }

   return(copy);
}


/* ###### Adapt pool element ################################################ */
uint16_t poolElementAdapt(struct PoolElement*       poolElement,
                          const struct PoolElement* source)
{
   GList*             transportAddressList;
   struct PoolPolicy* policy;

   policy = poolPolicyDuplicate(source->Policy);
   if(policy == NULL) {
      return(AEC_OUT_OF_MEMORY);
   }

   transportAddressList = transportAddressListDuplicate(source->TransportAddressList);
   if(transportAddressList == NULL) {
      poolPolicyDelete(policy);
      return(AEC_OUT_OF_MEMORY);
   }

   poolPolicyDelete(poolElement->Policy);
   poolElement->Policy = policy;

   transportAddressListDelete(poolElement->TransportAddressList);
   poolElement->TransportAddressList = transportAddressList;

   return(AEC_OKAY);
}


/* ###### Print PoolElement ############################################## */
void poolElementPrint(const struct PoolElement* poolElement, FILE* fd)
{
   GList* list;

   if(poolElement != NULL) {
      fprintf(fd,"Pool Element $%08x",poolElement->Identifier);
      if(poolElement->OwnerPool) {
         fprintf(fd," of pool ");
         poolHandlePrint(poolElement->OwnerPool->Handle,fd);
      }
      fprintf(fd,":\n");
      fprintf(fd,"   Home ENRP ID  = $%08x\n",  poolElement->HomeENRPServerID);
      fprintf(fd,"   Reg. Lifetime = %d [s]\n", poolElement->RegistrationLife);
      fprintf(fd,"   Control Ch.   = %s\n", (poolElement->HasControlChannel) ? "yes" : "no");
      fprintf(fd,"   Flags         = ");
      if(poolElement->Flags == 0) {
         fprintf(fd,"none");
      }
      else {
         if(poolElement->Flags & PEF_FAILED) {
            fprintf(fd,"PEF_FAILED ");
         }
      }
      fprintf(fd,"\n");
      fprintf(fd,"   UserCounter   = %d\n",poolElement->UserCounter);
      poolPolicyPrint(poolElement->Policy,fd);

      list = g_list_first(poolElement->TransportAddressList);
      while(list != NULL) {
         fprintf(fd,"      ");
         transportAddressPrint((struct TransportAddress*)list->data,fd);
         fprintf(fd,"\n");
         list = g_list_next(list);
      }
   }
   else {
      fputs("Pool Element (null)",fd);
   }
}


/* ###### Add transport address ########################################## */
void poolElementAddTransportAddress(struct PoolElement* poolElement, struct TransportAddress* transportAddress)
{
   if((poolElement != NULL) && (transportAddress != NULL)) {

      LOG_VERBOSE5
      fprintf(stdlog,"Adding address ");
      transportAddressPrint(transportAddress,stdlog);
      fprintf(stdlog," to pool element $%08x",
              poolElement->Identifier);
      if(poolElement->OwnerPool) {
         fprintf(stdlog," of pool ");
         poolHandlePrint(poolElement->OwnerPool->Handle,stdlog);
      }
      fputs("\n",stdlog);
      LOG_END

      poolElement->TransportAddressList = g_list_append(
         poolElement->TransportAddressList,
         (gpointer)transportAddress);
      poolElement->TransportAddresses++;
   }
}


/* ###### Remove transport address ####################################### */
void poolElementRemoveTransportAddress(struct PoolElement* poolElement, struct TransportAddress* transportAddress)
{
   if((poolElement != NULL) && (transportAddress != NULL)) {
      LOG_VERBOSE5
      fprintf(stdlog,"Removing address ");
      transportAddressPrint(transportAddress,stdlog);
      fprintf(stdlog," from pool element $%08x",
              poolElement->Identifier);
      if(poolElement->OwnerPool) {
         fprintf(stdlog," of pool ");
         poolHandlePrint(poolElement->OwnerPool->Handle,stdlog);
      }
      fputs("\n",stdlog);
      LOG_END

      poolElement->TransportAddressList = g_list_remove(poolElement->TransportAddressList,transportAddress);
      poolElement->TransportAddresses--;
   }
}


/* ###### PoolElement comparision function ############################### */
gint poolElementCompareFunc(gconstpointer a,
                            gconstpointer b)
{
   const struct PoolElement* e1 = (struct PoolElement*)a;
   const struct PoolElement* e2 = (struct PoolElement*)b;

   return(e1->Identifier == e2->Identifier);
}
