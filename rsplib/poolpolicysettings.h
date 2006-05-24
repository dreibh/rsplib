/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
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

#ifndef POOLELEMENTPOLICYSETTINGS_H
#define POOLELEMENTPOLICYSETTINGS_H


#include <stdio.h>

#include "rserpool-policytypes.h"


#ifdef __cplusplus
extern "C" {
#endif


struct PoolPolicySettings
{
   unsigned int PolicyType;
   unsigned int Weight;
   unsigned int Load;
   unsigned int LoadDegradation;
   unsigned int LoadDPF;
   unsigned int WeightDPF;
   unsigned int Distance;
};


#define PEPS_MIN_WEIGHT                   0
#define PEPS_MAX_WEIGHT          0xffffffff
#define PEPS_MIN_LOAD                     0
#define PEPS_MAX_LOAD              0xffffff
#define PEPS_MIN_LOADDEGRADATION          0
#define PEPS_MAX_LOADDEGRADATION   0xffffff


void poolPolicySettingsNew(struct PoolPolicySettings* pps);
void poolPolicySettingsDelete(struct PoolPolicySettings* pps);
void poolPolicySettingsPrint(const struct PoolPolicySettings* pps,
                             FILE*                            fd);
void poolPolicySettingsGetDescription(const struct PoolPolicySettings* pps,
                                      char*                            buffer,
                                      const size_t                     bufferSize);
int poolPolicySettingsComparison(const struct PoolPolicySettings* pps1,
                                 const struct PoolPolicySettings* pps2);
int poolPolicySettingsIsValid(const struct PoolPolicySettings* pps);
int poolPolicySettingsAdapt(struct PoolPolicySettings* pps,
                            const unsigned int         destinationType);


#ifdef __cplusplus
}
#endif


#endif
