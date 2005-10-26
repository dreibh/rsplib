/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2005 by Thomas Dreibholz
 *
 * $Id$
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

#ifndef POOLHANDLESPACEMANAGEMENT_BASICS_H
#define POOLHANDLESPACEMANAGEMENT_BASICS_H


#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "rserpoolerror.h"


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_PE_TRANSPORTADDRESSES 32


typedef uint32_t RegistrarIdentifierType;
typedef uint32_t PoolElementIdentifierType;
typedef uint32_t PoolElementSeqNumberType;

#define UNDEFINED_POOL_ELEMENT_IDENTIFIER 0
#define UNDEFINED_REGISTRAR_IDENTIFIER    0


#define PENPO_POLICYINFO             (1 << 0)   /* constants set by PE      */
#define PENPO_POLICYSTATE            (1 << 1)   /* current policy state     */
#define PENPO_HOME_PR                (1 << 2)   /* Home PR identifier       */
#define PENPO_REGLIFE                (1 << 3)   /* Registration lifetime    */
#define PENPO_UR_REPORTS             (1 << 4)   /* Unreachability reports   */
#define PENPO_LASTUPDATE             (1 << 5)   /* Last update time stamp   */
#define PENPO_USERTRANSPORT          (1 << 6)   /* User transport           */
#define PENPO_REGISTRATORTRANSPORT   (1 << 7)   /* Registrator transport    */
#define PENPO_CONNECTION             (1 << 8)   /* Connection (sd/assoc)    */
#define PENPO_CHECKSUM               (1 << 9)   /* PE entry checksum        */

#define PNPO_INDEX                   (1 << 16)  /* PEs of pool by index     */
#define PNPO_SELECTION               (1 << 17)  /* PEs of pool by selection */

#define PNNPO_POOLS_INDEX            (1 << 24)
#define PNNPO_POOLS_SELECTION        (1 << 25)
#define PNNPO_POOLS_OWNERSHIP        (1 << 26)
#define PNNPO_POOLS_CONNECTION       (1 << 27)
#define PNNPO_POOLS_TIMER            (1 << 28)


#define PENPO_FULL          (~0)
#define PENPO_ONLY_ID       0
#define PENPO_ONLY_POLICY   (PENPO_POLICYINFO|PENPO_POLICYSTATE|PENPO_UR_REPORTS|PNNPO_POOLS_SELECTION)



#define PLNPO_TRANSPORT     (1 << 0)

#define PLPO_PEERS_INDEX    (1 << 10)
#define PLPO_PEERS_ADDRESS  (1 << 11)
#define PLPO_PEERS_TIMER    (1 << 12)

#define PLPO_FULL           (~0)
#define PLPO_ONLY_INDEX     (PLPO_PEERS_INDEX)


enum PoolNodeUpdateAction
{
   PNUA_Create = 0x01,
   PNUA_Delete = 0x02,
   PNUA_Update = 0x03
};


const char* poolHandlespaceManagementGetErrorDescription(const unsigned int errorCode);
PoolElementIdentifierType getPoolElementIdentifier();


/*
 Starting value for seqence numbers. Set it to (~0) ^ 0xf to test
 sequence number warp.
 */
extern const PoolElementSeqNumberType SeqNumberStart;


#ifdef __cplusplus
}
#endif

#endif
