/*
 *  $Id: asaperror.c,v 1.2 2004/07/20 08:47:38 dreibh Exp $
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
 * Purpose: ASAP Error Definitions
 *
 */


#include "tdtypes.h"
#include "asaperror.h"



struct ErrorTable
{
   enum ASAPError ErrorCode;
   const char*    ErrorText;
};

static struct ErrorTable ErrorDescriptions[] = {
   { ASAP_Okay,                      "Okay" },
   { ASAP_UnrecognizedMessage,       "Unrecognized message" },
   { ASAP_UnrecognizedParameter,     "Unrecognised parameter" },
   { ASAP_AuthorizationFailure,      "Authorization failure" },
   { ASAP_InvalidValues,             "Invalid values" },
   { ASAP_NonUniquePEID,             "Non-unique pool element identifier" },
   { ASAP_PolicyInconsistent,        "Policy inconsistent" },
   { ASAP_BufferSizeExceeded,        "Buffer size exceeded" },
   { ASAP_OutOfMemory,               "Out of memory" },
   { ASAP_ReadError,                 "Read error" },
   { ASAP_WriteError,                "Write error" },
   { ASAP_ConnectionFailureUnusable, "Connection failure, unusable address type" },
   { ASAP_ConnectionFailureSocket,   "Connection failure, socket() call failed" },
   { ASAP_ConnectionFailureConnect,  "Connection failure, connect() call failed" },
   { ASAP_NotFound,                  "No pool element(s) found" },
   { ASAP_NoNameServerFound,         "No name server found" }
};



/* ###### Get error description ########################################## */
const char* asapErrorGetDescription(const enum ASAPError error)
{
   const size_t size = sizeof(ErrorDescriptions) / sizeof(struct ErrorTable);
   size_t       i;

   for(i = 0;i < size;i++) {
      if(ErrorDescriptions[i].ErrorCode == error) {
         return(ErrorDescriptions[i].ErrorText);
      }
   }
   return("Unknown error");
}


/* ###### Print error description ######################################## */
void asapErrorPrint(const enum ASAPError error, FILE* fd)
{
   fprintf(fd,"$%04x - %s",error,asapErrorGetDescription(error));
}
