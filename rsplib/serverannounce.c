/*
 *  $Id: serverannounce.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Server Announce
 *
 */


#include "tdtypes.h"
#include "serverannounce.h"
#include "utilities.h"



/* ###### Constructor #################################################### */
struct ServerAnnounce* serverAnnounceNew(GList*             transportAddressList,
                                         const unsigned int flags)
{
   struct ServerAnnounce* serverAnnounce = (struct ServerAnnounce*)malloc(sizeof(struct ServerAnnounce));
   if(serverAnnounce != NULL) {
      serverAnnounce->Flags                = flags;
      serverAnnounce->TransportAddressList = transportAddressList;
      serverAnnounce->LastUpdate           = getMicroTime();
   }
   return(serverAnnounce);
}


/* ###### Destructor ##################################################### */
void serverAnnounceDelete(struct ServerAnnounce* serverAnnounce)
{
   if(serverAnnounce != NULL) {
      if(serverAnnounce->TransportAddressList) {
         transportAddressListDelete(serverAnnounce->TransportAddressList);
         serverAnnounce->TransportAddressList = NULL;
      }
      free(serverAnnounce);
   }
}


/* ###### Duplicate ServerAnnounce ########################################### */
struct ServerAnnounce* serverAnnounceDuplicate(struct ServerAnnounce* serverAnnounce)
{
   struct ServerAnnounce* copy = NULL;
   GList*             transportAddressList;

   if(serverAnnounce != NULL) {
      transportAddressList = transportAddressListDuplicate(serverAnnounce->TransportAddressList);
      if(transportAddressList != NULL) {
         copy = serverAnnounceNew(serverAnnounce->TransportAddressList,
                              serverAnnounce->Flags);
         if(copy == NULL) {
            transportAddressListDelete(transportAddressList);
         }
      }
   }
   return(copy);
}


/* ###### Print ServerAnnounce ############################################### */
void serverAnnouncePrint(struct ServerAnnounce* serverAnnounce, FILE* fd)
{
   struct TransportAddress* transportAddress;
   GList*                   transportAddressList;

   if(serverAnnounce != NULL) {
      fputs("Server Announce:\n",fd);
      fprintf(fd,"   Last Update = %1.3f [s] ago\n",
         (getMicroTime() - serverAnnounce->LastUpdate) / 1000000.0);
      fputs("   Flags       =",fd);
      if(serverAnnounce->Flags & SIF_DYNAMIC) {
         fputs(" Dynamic",fd);
      }
      else {
         fputs(" Static",fd);
      }
      fputs("\n",fd);
      fputs("   Addresses   =\n",fd);
      transportAddressList = g_list_first(serverAnnounce->TransportAddressList);
      while(transportAddressList != NULL) {
         transportAddress = (struct TransportAddress*)transportAddressList->data;
         fputs("   ", fd);
         transportAddressPrint(transportAddress, fd);
         fputs("\n",fd);
         transportAddressList = g_list_next(transportAddressList);
      }
   }
   else {
      fputs("   (null)\n", fd);
   }
}


/* ###### ServerAnnounce comparision function ################################ */
gint serverAnnounceCompareFunc(gconstpointer a, gconstpointer b)
{
   const struct ServerAnnounce* s1 = (const struct ServerAnnounce*)a;
   const struct ServerAnnounce* s2 = (const struct ServerAnnounce*)b;

   const gint result = transportAddressListCompareFunc((gpointer)s1->TransportAddressList,
                                                       (gpointer)s2->TransportAddressList);
   return(result);
}
