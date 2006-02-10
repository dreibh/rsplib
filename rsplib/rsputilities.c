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

#include "tdtypes.h"
#include "rserpool.h"
#include "loglevel.h"
#include "netutilities.h"
#include "stringutilities.h"
#include "debug.h"


/* ###### Create new static registrar entry in rsp_info ###### */
#define MAX_PR_TRANSPORTADDRESSES 128
int addStaticRegistrar(struct rsp_info* info,
                       const char*      addressString)
{
   union sockaddr_union       addressArray[MAX_PR_TRANSPORTADDRESSES];
   struct sockaddr*           packedAddressArray;
   struct rsp_registrar_info* registrarInfo;
   char                       str[1024];
   size_t                     addresses;
   char*                      address;
   char*                      idx;

   safestrcpy((char*)&str, addressString, sizeof(str));
   addresses = 0;
   address = str;
   while(addresses < MAX_PR_TRANSPORTADDRESSES) {
      idx = strindex(address, ',');
      if(idx) {
         *idx = 0x00;
      }
      if(!string2address(address, &addressArray[addresses])) {
         return(-1);
      }
      addresses++;
      if(idx) {
         address = idx;
         address++;
      }
      else {
         break;
      }
   }
   if(addresses < 1) {
      return(-1);
   }

   packedAddressArray = pack_sockaddr_union((union sockaddr_union*)&addressArray, addresses);
   if(packedAddressArray == NULL) {
      return(-1);
   }

   registrarInfo = (struct rsp_registrar_info*)malloc(sizeof(struct rsp_registrar_info));
   if(registrarInfo == NULL) {
      free(packedAddressArray);
      return(-1);
   }

   registrarInfo->rri_next  = info->ri_registrars;
   registrarInfo->rri_addr  = packedAddressArray;
   registrarInfo->rri_addrs = addresses;
   info->ri_registrars = registrarInfo;
   return(0);
}


/* ###### Free static registrar entries of rsp_info ###################### */
void freeStaticRegistrars(struct rsp_info* info)
{
   struct rsp_registrar_info* registrarInfo;
   while(info->ri_registrars) {
      registrarInfo = info->ri_registrars;
      info->ri_registrars = registrarInfo->rri_next;
      free(registrarInfo->rri_addr);
      free(registrarInfo);
   }
}


#ifdef ENABLE_CSP
/* ###### Set logging parameter ########################################## */
static union sockaddr_union cspServerAddress;
bool initComponentStatusReporter(struct rsp_info* info,
                                 const char*      parameter)
{
   if(!(strncmp(parameter, "-cspserver=", 11))) {
      if(!string2address((const char*)&parameter[11], &cspServerAddress)) {
         fprintf(stderr,
                  "ERROR: Bad CSP report address %s! Use format <address:port>.\n",
                  (const char*)&parameter[11]);
         return(false);
      }
      info->ri_csp_server = &cspServerAddress.sa;
   }
   else if(!(strncmp(parameter, "-cspinterval=", 13))) {
      info->ri_csp_interval = atol((const char*)&parameter[13]);
      return(true);
   }
   else {
      fprintf(stderr, "ERROR: Invalid CSP parameter %s\n", parameter);
      return(false);
   }
   return(true);
}
#endif
