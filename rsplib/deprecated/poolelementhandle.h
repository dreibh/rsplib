/*
 *  $Id: poolelementhandle.h,v 1.1 2004/07/13 09:12:10 dreibh Exp $
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
 * Purpose: Pool Element Handle
 *
 */


#ifndef POOLELEMENTHANDLE_H
#define POOLELEMENTHANDLE_H


#include "tdtypes.h"

#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef uint64_t PoolElementHandle;


#define UNDEFINED_POOL_ELEMENT_HANDLE 0


/**
  * Get new pool element handle.
  *
  * @return Pool element handle.
  */
PoolElementHandle getPoolElementHandle();

/**
  * Pool element handle hash function.
  *
  * @param key Pointer to pool element handle.
  * @return Hash value.
  */
guint poolElementHandleHashFunc(gconstpointer key);

/**
  * Pool element handle comparision function.
  *
  * @param a Pointer to pool element handle 1.
  * @param b Pointer to pool element handle 2.
  * @return Comparision result.
  */
gint poolElementHandleCompareFunc(gconstpointer a,
                                  gconstpointer b);

/**
  * Pool element handle equality function.
  *
  * @param a Pointer to pool element handle 1.
  * @param b Pointer to pool element handle 2.
  * @return <>0 if a equal to b, 0 otherwise.
  */
gint poolElementHandleEqualFunc(gconstpointer a,
                                gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
