/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2021 by Thomas Dreibholz
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

#include "rserpoolerror.h"

#include <ctype.h>


struct ErrorTable
{
   const unsigned short ErrorCode;
   const char*          ErrorText;
};

static struct ErrorTable ErrorDescriptions[] = {
   /* No error */
   { RSPERR_OKAY,                          "okay!" },

   /* Protocol-specific error causes */
   { RSPERR_UNRECOGNIZED_PARAMETER,        "unrecognized parameter" },
   { RSPERR_UNRECOGNIZED_MESSAGE,          "unrecognized message" },
   { RSPERR_INVALID_MESSAGE,               "invalid message" },
   { RSPERR_INVALID_TLV,                   "invalid TLV" },
   { RSPERR_INVALID_VALUE,                 "invalid value" },
   { RSPERR_NO_USABLE_USER_ADDRESSES,      "no usable user endpoint address(es)" },
   { RSPERR_NO_USABLE_ASAP_ADDRESSES,      "no usable ASAP endpoint address(es)" },

   /* Implementation-specific error causes */
   { RSPERR_NOT_INITIALIZED,               "not initialized" },
   { RSPERR_BUFFERSIZE_EXCEEDED,           "buffer size exceeded (message too long)" },
   { RSPERR_READ_ERROR,                    "read() error" },
   { RSPERR_WRITE_ERROR,                   "write() error" },
   { RSPERR_CONNECTION_FAILURE_SOCKET,     "socket() failure" },
   { RSPERR_CONNECTION_FAILURE_CONNECT,    "connect() failure" },
   { RSPERR_NO_REGISTRAR,                  "no registrar available" },
   { RSPERR_TIMEOUT,                       "timeout" },
   { RSPERR_UNRECOGNIZED_PARAMETER_SILENT, "unrecognized parameter (silent)" },

   /* Handlespace-management specific error causes */
   { RSPERR_OUT_OF_RESOURCES,              "out of resources" },
   { RSPERR_NOT_FOUND,                     "object not found" },
   { RSPERR_INVALID_ID,                    "invalid ID" },
   { RSPERR_OWN_ID,                        "own ID" },
   { RSPERR_DUPLICATE_ID,                  "duplicate ID" },
   { RSPERR_WRONG_PROTOCOL,                "wrong protocol" },
   { RSPERR_WRONG_CONTROLCHANNEL_HANDLING, "wrong control channel handling" },
   { RSPERR_INCOMPATIBLE_POOL_POLICY,      "incompatible pool policy" },
   { RSPERR_UNSUPPORTED_POOL_POLICY,       "unsupported pool policy" },
   { RSPERR_INVALID_POOL_POLICY,           "invalid pool policy" },
   { RSPERR_INVALID_POOL_HANDLE,           "invalid pool handle (too long)" },
   { RSPERR_INVALID_REGISTRATOR,           "invalid registrator" },
   { RSPERR_INVALID_ADDRESSES,             "invalid address(es)" }
};



/* ###### Get error description ########################################## */
const char* rserpoolErrorGetDescription(const unsigned int error)
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
void rserpoolErrorPrint(const unsigned int error,
                        FILE*              fd)
{
   fputs(rserpoolErrorGetDescription(error), fd);
}
