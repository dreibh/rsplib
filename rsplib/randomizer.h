/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
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

#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include "tdtypes.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif



/**
  * Get 8-bit random value.
  *
  * @return Random value.
  */
uint8_t random8();

/**
  * Get162-bit random value.
  *
  * @return Random value.
  */
uint16_t random16();

/**
  * Get 32-bit random value.
  *
  * @return Random value.
  */
uint32_t random32();

/**
  * Get 64-bit random value.
  *
  * @return Random value.
  */
uint64_t random64();

/**
  * Get double random value out of interval [0,1).
  *
  * @return Random value.
  */
double randomExpDouble(const double p);

/**
  * Get exponential-distributed random value.
  *
  * @param p Average value.
  * @return Random value.
  */
double randomExpDouble(const double p);


#ifdef __cplusplus
}
#endif


#endif
