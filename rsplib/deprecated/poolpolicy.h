/*
 *  $Id: poolpolicy.h,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Pool Policy
 *
 */


#ifndef POOLPOLICY_H
#define POOLPOLICY_H


#include "tdtypes.h"
#include "transportaddress.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



#define PPT_ROUNDROBIN            0x01
#define PPT_LEASTUSED             0x02
#define PPT_LEASTUSED_DEGRADATION 0x03
#define PPT_WEIGHTED_ROUNDROBIN   0x04
#define PPT_RANDOM                0xfe
#define PPT_WEIGHTED_RANDOM       0xff



struct PoolPolicy
{
   uint8_t  Type;
   cardinal Weight;
   cardinal Load;
};



/**
  * Constructor.
  *
  * @param type Policy type.
  * @return PoolPolicy or NULL in case of error.
  */
struct PoolPolicy* poolPolicyNew(const uint8_t type);

/**
  * Destructor.
  *
  * @param poolPolicy PoolPolicy.
  */
void poolPolicyDelete(struct PoolPolicy* poolPolicy);

/**
  * Duplicate pool policy.
  *
  * @param source PoolPolicy to be duplicated.
  * @return Copy of pool policy or NULL in case of error.
  */
struct PoolPolicy* poolPolicyDuplicate(const struct PoolPolicy* source);

/**
  * Print pool policy.
  *
  * @param poolPolicy PoolPolicy.
  * @param fd File to write pool handle to (e.g. stdout, stderr, ...).
  */
void poolPolicyPrint(const struct PoolPolicy* poolPolicy, FILE* fd);

/**
  * Adapt pool policy to another policy.
  *
  * @param poolPolicy PoolPolicy to be adapted.
  * @param source PoolPolicy to which should be adapted to.
  * @return true for success; false otherwise.
  */
bool poolPolicyAdapt(struct PoolPolicy* poolPolicy, const struct PoolPolicy* source);



#ifdef __cplusplus
}
#endif


#endif
