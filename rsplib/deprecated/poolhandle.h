/*
 *  $Id: poolhandle.h,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Pool Handle
 *
 */


#ifndef POOLHANDLE_H
#define POOLHANDLE_H


#include "tdtypes.h"
#include "poolelement.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



struct PoolHandle
{
   size_t Length;
   char   Handle[0];
};



/**
  * Constructor.
  *
  * @param handle Pool handle.
  * @param length Length of pool handle.
  * @return PoolHandle or NULL in case of error.
  */
struct PoolHandle* poolHandleNew(const char* handle, const size_t length);

/**
  * Constructor for ASCII pool handle.
  *
  * @param handle Pool handle string.
  * @return PoolHandle or NULL in case of error.
  */
struct PoolHandle* poolHandleNewASCII(const char* poolName);

/**
  * Destructor.
  *
  * @param poolHandle PoolHandle.
  */
void poolHandleDelete(struct PoolHandle* poolHandle);

/**
  * Duplicate pool handle.
  *
  * @param source PoolHandle to be duplicated.
  * @return Copy of pool handle or NULL in case of error.
  */
struct PoolHandle* poolHandleDuplicate(const struct PoolHandle* source);

/**
  * Print pool handle.
  *
  * @param poolHandle PoolHandle.
  * @param fd File to write pool handle to (e.g. stdout, stderr, ...).
  */
void poolHandlePrint(const struct PoolHandle* poolHandle, FILE* fd);

/**
  * Pool handle hash function.
  *
  * @param key Pointer to pool handle.
  * @return Hash value.
  */
guint poolHandleHashFunc(gconstpointer key);

/**
  * Pool handle comparision function.
  *
  * @param a Pointer to pool handle 1.
  * @param b Pointer to pool handle 2.
  * @return Comparision result.
  */
gint poolHandleCompareFunc(gconstpointer a,
                           gconstpointer b);

/**
  * Pool handle equality function.
  *
  * @param a Pointer to pool handle 1.
  * @param b Pointer to pool handle 2.
  * @return <>0 if a equal to b, 0 otherwise.
  */
gint poolHandleEqualFunc(gconstpointer a,
                         gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
