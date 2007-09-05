/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2007 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include "tdtypes.h"
#include "rserpool.h"
#include "loglevel.h"
#include "netutilities.h"
#include "stringutilities.h"
#include "asapinstance.h"
#include "debug.h"
#ifdef ENABLE_CSP
#include "componentstatuspackets.h"
#endif


/* ###### Create new static registrar entry in rsp_info ###### */
#define MAX_PR_TRANSPORTADDRESSES 128
static int addStaticRegistrar(struct rsp_info* info,
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


#ifdef ENABLE_CSP
/* ###### Set logging parameter ########################################## */
static bool initComponentStatusReporter(struct rsp_info* info,
                                        const char*      parameter)
{
   static union sockaddr_union cspServerAddress;
   unsigned int                identifier;

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
   else if(!(strncmp(parameter, "-cspidentifier=", 15))) {
      if(sscanf((const char*)&parameter[15], "0x%x", &identifier) == 0) {
         if(sscanf((const char*)&parameter[15], "%u", &identifier) == 0) {
            fputs("ERROR: Bad registrar ID given!\n", stderr);
            exit(1);
         }
      }
      info->ri_csp_identifier = CID_COMPOUND(CID_GROUP_POOLELEMENT, identifier);
   }
   else {
      fprintf(stderr, "ERROR: Invalid CSP parameter %s\n", parameter);
      return(false);
   }
   return(true);
}
#endif


/* ###### Initialize rsp_info ############################################ */
void rsp_initinfo(struct rsp_info* info)
{
#ifdef ENABLE_CSP
   const char*                 cspServer   = getenv("CSP_SERVER");
   const char*                 cspInterval = getenv("CSP_INTERVAL");
   static union sockaddr_union cspServerAddress;
#endif
   memset(info, 0, sizeof(struct rsp_info));
#ifdef ENABLE_CSP
   if(cspServer) {
      if(!string2address(cspServer, &cspServerAddress)) {
         fprintf(stderr,
                  "ERROR: Bad CSP report address %s! Use format <address:port>.\n",
                  cspServer);
      }
      info->ri_csp_server = &cspServerAddress.sa;
   }
   if(cspInterval) {
      info->ri_csp_interval = atol(cspInterval);
      if(info->ri_csp_interval < 250) {
         info->ri_csp_interval = 250;
      }
   }
#endif
}


/* ###### Free static registrar entries of rsp_info ###################### */
void rsp_freeinfo(struct rsp_info* info)
{
   struct rsp_registrar_info* registrarInfo;
   while(info->ri_registrars) {
      registrarInfo = info->ri_registrars;
      info->ri_registrars = registrarInfo->rri_next;
      free(registrarInfo->rri_addr);
      free(registrarInfo);
   }
}


/* ###### Handle rsplib argument and put result into rsp_info ############ */
int rsp_initarg(struct rsp_info* info, const char* arg)
{
   static union sockaddr_union asapAnnounceAddress;

   if(!(strncmp(arg, "-log" ,4))) {
      if(initLogging(arg) == false) {
         exit(1);
      }
      return(1);
   }
#ifdef ENABLE_CSP
   else if(!(strncmp(arg, "-csp" ,4))) {
      if(initComponentStatusReporter(info, arg) == false) {
         exit(1);
      }
      return(1);
   }
#endif
   else if(!(strncmp(arg, "-registrar=", 11))) {
      if(addStaticRegistrar(info, (char*)&arg[11]) < 0) {
         fprintf(stderr, "ERROR: Bad registrar setting %s\n", arg);
         exit(1);
      }
      return(1);
   }
   else if(!(strncmp(arg, "-asapannounce=", 14))) {
      if(!(strcasecmp((const char*)&arg[14], "auto"))) {
         info->ri_registrar_announce = NULL;
         info->ri_disable_autoconfig = 0;
      }
      else if(!(strcasecmp((const char*)&arg[14], "off"))) {
         info->ri_registrar_announce = NULL;
         info->ri_disable_autoconfig = 1;
      }
      else {
         if(string2address((const char*)&arg[14], &asapAnnounceAddress) == false) {
            fprintf(stderr, "ERROR: Bad ASAP announce setting %s\n", arg);
            exit(1);
         }
         info->ri_registrar_announce = (struct sockaddr*)&asapAnnounceAddress;
         info->ri_disable_autoconfig = 0;
      }
      return(1);
   }
   return(0);
}
