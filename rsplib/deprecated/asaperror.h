/*
 *  $Id: asaperror.h,v 1.1 2004/07/18 15:30:43 dreibh Exp $
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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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


#ifndef ASAPERROR_H
#define ASAPERROR_H


#include "tdtypes.h"
#include "asapmessage.h"


#ifdef __cplusplus
extern "C" {
#endif


enum ASAPError
{
   ASAP_Okay                      = AEC_OKAY,

   ASAP_UnrecognizedMessage       = AEC_UNRECOGNIZED_MESSAGE,
   ASAP_UnrecognizedParameter     = AEC_UNRECOGNIZED_PARAMETER,
   ASAP_AuthorizationFailure      = AEC_AUTHORIZATION_FAILURE,
   ASAP_InvalidValues             = AEC_INVALID_VALUES,
   ASAP_NonUniquePEID             = AEC_NONUNIQUE_PE_ID,
   ASAP_PolicyInconsistent        = AEC_POLICY_INCONSISTENT,


   ASAP_BufferSizeExceeded        = AEC_BUFFERSIZE_EXCEEDED,
   ASAP_OutOfMemory               = AEC_OUT_OF_MEMORY,


   ASAP_ReadError                 = AEC_READ_ERROR,
   ASAP_WriteError                = AEC_WRITE_ERROR,
   ASAP_ConnectionFailureUnusable = AEC_CONNECTION_FAILURE_UNUSABLE,
   ASAP_ConnectionFailureSocket   = AEC_CONNECTION_FAILURE_SOCKET,
   ASAP_ConnectionFailureConnect  = AEC_CONNECTION_FAILURE_CONNECT,

   ASAP_NotFound                  = AEC_NOT_FOUND,
   ASAP_NoNameServerFound         = AEC_NO_NAME_SERVER_FOUND
};



/**
  * Get text description for given error code.
  *
  * @param error ASAPError.
  * @return Text description for error.
  */
const char* asapErrorGetDescription(const enum ASAPError error);


/**
  * Print error.
  *
  * @param error ASAPError.
  * @param fd Output file (e.g. stdin, stderr, ...).
  */
void asapErrorPrint(const enum ASAPError error, FILE* fd);

#ifdef __cplusplus
}
#endif

#endif
