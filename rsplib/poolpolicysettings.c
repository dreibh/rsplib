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

#include "poolpolicysettings.h"


/* ###### Initialize ##################################################### */
void poolPolicySettingsNew(struct PoolPolicySettings* pps)
{
   pps->PolicyType      = 0;
   pps->Weight          = 0;
   pps->Load            = 0;
   pps->LoadDegradation = 0;
}


/* ###### Invalidate ##################################################### */
void poolPolicySettingsDelete(struct PoolPolicySettings* pps)
{
   pps->PolicyType      = 0;
   pps->Weight          = 0;
   pps->Load            = 0;
   pps->LoadDegradation = 0;
}


/* ###### Comparison ##################################################### */
int poolPolicySettingsComparison(const struct PoolPolicySettings* pps1,
                                        const struct PoolPolicySettings* pps2)
{
   return((pps1->Weight          != pps2->Weight) ||
          (pps1->Load            != pps2->Load)   ||
          (pps1->LoadDegradation != pps2->LoadDegradation));
}


/* ###### Check, if settings are valid ################################### */
int poolPolicySettingsIsValid(const struct PoolPolicySettings* pps)
{
   return( (pps->Weight >= PEPS_MIN_WEIGHT) &&
           (pps->Weight <= PEPS_MAX_WEIGHT) &&
           (pps->Load >= PEPS_MIN_LOAD) &&
           (pps->Load <= PEPS_MAX_LOAD) &&
           (pps->LoadDegradation >= PEPS_MIN_LOADDEGRADATION) &&
           (pps->LoadDegradation <= PEPS_MAX_LOADDEGRADATION) );
}


/* ###### Pool Policy adaptation ######################################### */
int poolPolicySettingsAdapt(struct PoolPolicySettings* pps,
                            const unsigned int         destinationType)

{
   return(pps->PolicyType == destinationType);
}


/* ###### Get textual description ######################################## */
void poolPolicySettingsGetDescription(const struct PoolPolicySettings* pps,
                                      char*                            buffer,
                                      const size_t                     bufferSize)
{
   snprintf(buffer, bufferSize,
            "w=%u l=$%x ldeg=$%x", pps->Weight,
                                   pps->Load,
                                   pps->LoadDegradation);
}


/* ###### Print ########################################################## */
void poolPolicySettingsPrint(const struct PoolPolicySettings* pps,
                             FILE*                            fd)
{
   char buffer[256];
   poolPolicySettingsGetDescription(pps, (char*)&buffer, sizeof(buffer));
   fputs(buffer, fd);
}
