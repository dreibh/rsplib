/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef RSERPOOLERROR_H
#define RSERPOOLERROR_H

#include <stdio.h>


/* Protocol-specific error causes */
#define RSPERR_UNRECOGNIZED_PARAMETER        0x0001
#define RSPERR_UNRECOGNIZED_MESSAGE          0x0002
#define RSPERR_AUTHORIZATION_FAILURE         0x0003
#define RSPERR_INVALID_VALUES                0x0004

/* Implementation-specific error causes */
#define RSPERR_BUFFERSIZE_EXCEEDED           0x1000
#define RSPERR_OUT_OF_MEMORY                 0x1001
#define RSPERR_READ_ERROR                    0x1010
#define RSPERR_WRITE_ERROR                   0x1011
#define RSPERR_CONNECTION_FAILURE_UNUSABLE   0x1012
#define RSPERR_CONNECTION_FAILURE_SOCKET     0x1013
#define RSPERR_CONNECTION_FAILURE_CONNECT    0x1014
#define RSPERR_NO_NAMESERVER                 0x1021

/* Namespace-management specific error causes */
#define RSPERR_OKAY                          0xf001
#define RSPERR_NO_RESOURCES                  0xf002
#define RSPERR_NOT_FOUND                     0xf003
#define RSPERR_INVALID_ID                    0xf004
#define RSPERR_DUPLICATE_ID                  0xf005
#define RSPERR_WRONG_PROTOCOL                0xf006
#define RSPERR_WRONG_CONTROLCHANNEL_HANDLING 0xf007
#define RSPERR_INCOMPATIBLE_POOL_POLICY      0xf008
#define RSPERR_INVALID_POOL_POLICY           0xf009
#define RSPERR_INVALID_POOL_HANDLE           0xf00a
#define RSPERR_INVALID_ADDRESSES             0xf00b


const char* rserpoolErrorGetDescription(const unsigned int error);
const char* rserpoolErrorPrint(const unsigned int error,
                               FILE*              fd);


#endif
