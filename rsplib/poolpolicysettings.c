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
