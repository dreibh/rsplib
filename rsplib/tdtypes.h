/*
 *  $Id$
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Foerderkennzeichen 01AK045).
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
 * Purpose: Type Definitions
 *
 */


#ifndef TDTYPES_H
#define TDTYPES_H


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef NDEBUG
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef FreeBSD
#include <machine/endian.h>
#include <sys/inttypes.h>
#endif
#ifdef DARWIN
#include <machine/endian.h>
#include <stdint.h>
#endif


#ifndef max
#define max(a, b) ((a) < (b) ? (b) : (a))
#endif
#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

/**
  * Datatype for storing a signed char.
  */
typedef int8_t sbyte;

/**
  * Datatype for storing an unsigned char.
  */
typedef uint8_t ubyte;

/**
  * Datatype for storing an 8-bit integer.
  */
typedef int8_t int8;

/**
  * Datatype for storing a 8-bit cardinal.
  */
typedef uint8_t card8;

/**
  * Datatype for storing a 16-bit integer.
  */
typedef int16_t int16;

/**
  * Datatype for storing a 16-bit cardinal.
  */
typedef uint16_t card16;

/**
  * Datatype for storing a 32-bit intger.
  */
typedef int32_t int32;

/**
  * Datatype for storing a default-sized integer (32 bits minimum).
  */
#if defined (int_least32_t)
typedef int_least32_t integer;
#else
typedef int32 integer;
#endif

/**
  * Datatype for storing a 32-bit cardinal.
  */
typedef uint32_t card32;

/**
  * Datatype for storing an 64-bit integer.
  */
typedef int64_t int64;

/**
  * Datatype for storing a 64-bit cardinal.
  */
typedef uint64_t card64;

/**
  * Datatype for storing a default-sized cardinal (32 bits minimum).
  */
#if defined (uint_least32_t)
typedef uint_least32_t cardinal;
#else
typedef card32 cardinal;
#endif


#ifndef __cplusplus
typedef unsigned char bool;
#define true 1
#define false 0
#endif


#ifndef HAVE_SOCKLEN_T
// typedef int socklen_t;
#endif


#endif
