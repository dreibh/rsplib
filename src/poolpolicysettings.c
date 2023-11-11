/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2024 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: thomas.dreibholz@gmail.com
 */

#include "poolpolicysettings.h"


/* ###### Initialize ##################################################### */
void poolPolicySettingsNew(struct PoolPolicySettings* pps)
{
   pps->PolicyType      = 0;
   pps->Weight          = 0;
   pps->Load            = 0;
   pps->LoadDegradation = 0;
   pps->LoadDPF         = 0;
   pps->WeightDPF       = 0;
   pps->Distance        = 0;
}


/* ###### Invalidate ##################################################### */
void poolPolicySettingsDelete(struct PoolPolicySettings* pps)
{
   pps->PolicyType      = 0;
   pps->Weight          = 0;
   pps->Load            = 0;
   pps->LoadDegradation = 0;
   pps->LoadDPF         = 0;
   pps->WeightDPF       = 0;
   pps->Distance        = 0;
}


/* ###### Comparison ##################################################### */
int poolPolicySettingsComparison(const struct PoolPolicySettings* pps1,
                                 const struct PoolPolicySettings* pps2)
{
   return((pps1->Weight          != pps2->Weight)          ||
          (pps1->Load            != pps2->Load)            ||
          (pps1->LoadDegradation != pps2->LoadDegradation) ||
          (pps1->LoadDPF         != pps2->LoadDPF)         ||
          (pps1->WeightDPF       != pps2->WeightDPF)       ||
          (pps1->Distance        != pps2->Distance));
}


/* ###### Check, if settings are valid ################################### */
int poolPolicySettingsIsValid(const struct PoolPolicySettings* pps)
{
   return( (pps->Weight >= PPV_MIN_WEIGHT) &&
           (pps->Weight <= PPV_MAX_WEIGHT) &&
           (pps->Load >= PPV_MIN_LOAD) &&
           (pps->Load <= PPV_MAX_LOAD) &&
           (pps->LoadDegradation >= PPV_MIN_LOAD_DEGRADATION) &&
           (pps->LoadDegradation <= PPV_MAX_LOAD_DEGRADATION) );
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
            "t=$%02x [w=%u l=%1.3f%%($%08x) ldeg=%1.3f%%($%08x) ldpf=%1.6f($%08x) wdpf=%1.6f($%08x) dist=%u]",
            pps->PolicyType,
            pps->Weight,
            (100.0 * pps->Load) / (double)(long long)PPV_MAX_LOAD, pps->Load,
            (100.0 * pps->LoadDegradation) / (double)(long long)PPV_MAX_LOAD_DEGRADATION, pps->LoadDegradation,
            pps->LoadDPF / (double)(long long)PPV_MAX_LOAD, pps->LoadDPF,
            pps->WeightDPF / (double)PPV_MAX_WEIGHTDPF, pps->WeightDPF,
            pps->Distance);
}


/* ###### Print ########################################################## */
void poolPolicySettingsPrint(const struct PoolPolicySettings* pps,
                             FILE*                            fd)
{
   char buffer[256];
   poolPolicySettingsGetDescription(pps, (char*)&buffer, sizeof(buffer));
   fputs(buffer, fd);
}
