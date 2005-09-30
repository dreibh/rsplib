/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
#include "asapinterthreadmessage.h"


/* ###### Constructor #################################################### */
struct ASAPInterThreadMessage* asapInterThreadMessageNew(
                                  struct RSerPoolMessage* request,
                                  const bool              responseExpected)
{
   struct ASAPInterThreadMessage* aitm;

   aitm = (struct ASAPInterThreadMessage*)malloc(sizeof(struct ASAPInterThreadMessage));
   if(aitm == NULL) {
      return(NULL);
   }

   aitm->Request                  = request;
   aitm->Response                 = NULL;
   aitm->ResponseExpected         = responseExpected;   /* Response from registrar? */
   aitm->Error                    = RSPERR_OKAY;
   aitm->TransmissionTrials       = 0;
   aitm->ResponseTimeoutTimeStamp = 0;
   return(aitm);
}


/* ###### Destructor ##################################################### */
void asapInterThreadMessageDelete(struct ASAPInterThreadMessage* aitm)
{
   if(aitm) {
      rserpoolMessageDelete(aitm->Request);
      aitm->Request = NULL;
      if(aitm->Response) {
         rserpoolMessageDelete(aitm->Response);
         aitm->Response = NULL;
      }
      free(aitm);
   }
}
