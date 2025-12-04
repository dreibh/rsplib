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
 * Copyright (C) 2003-2026 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#ifndef RSERPOOLERROR_H
#define RSERPOOLERROR_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/* No error */
#define RSPERR_OKAY                               0

/* Protocol-specific error causes */
#define RSPERR_UNRECOGNIZED_PARAMETER        0x0001
#define RSPERR_UNRECOGNIZED_MESSAGE          0x0002
#define RSPERR_INVALID_MESSAGE               0x0010
#define RSPERR_INVALID_TLV                   0x0003
#define RSPERR_INVALID_VALUE                 0x0011
#define RSPERR_INCOMPATIBLE_POOL_POLICY      0x0005
#define RSPERR_OUT_OF_RESOURCES              0x0006
#define RSPERR_WRONG_CONTROLCHANNEL_HANDLING 0x0008
#define RSPERR_NOT_FOUND                     0x0009

/* Implementation-specific error causes */
#define RSPERR_NOT_INITIALIZED               0x1000
#define RSPERR_BUFFERSIZE_EXCEEDED           0x1001
#define RSPERR_READ_ERROR                    0x1003
#define RSPERR_WRITE_ERROR                   0x1004
#define RSPERR_CONNECTION_FAILURE_SOCKET     0x1005
#define RSPERR_CONNECTION_FAILURE_CONNECT    0x1006
#define RSPERR_NO_REGISTRAR                  0x1007
#define RSPERR_TIMEOUT                       0x1008
#define RSPERR_UNRECOGNIZED_PARAMETER_SILENT 0x1009
#define RSPERR_WRONG_PROTOCOL                0x100a
#define RSPERR_UNSUPPORTED_POOL_POLICY       0x100b

/* Handlespace-management specific error causes */
#define RSPERR_INVALID_ID                    0xf000
#define RSPERR_OWN_ID                        0xf001
#define RSPERR_DUPLICATE_ID                  0xf002
#define RSPERR_INVALID_POOL_POLICY           0xf003
#define RSPERR_INVALID_POOL_HANDLE           0xf004
#define RSPERR_INVALID_ADDRESSES             0xf005
#define RSPERR_INVALID_REGISTRATOR           0xf006
#define RSPERR_NO_USABLE_ASAP_ADDRESSES      0xf007
#define RSPERR_NO_USABLE_USER_ADDRESSES      0xf008
#define RSPERR_OUT_OF_MEMORY                 RSPERR_OUT_OF_RESOURCES


const char* rserpoolErrorGetDescription(const unsigned int error);
void rserpoolErrorPrint(const unsigned int error,
                        FILE*              fd);


#ifdef __cplusplus
}
#endif

#endif
