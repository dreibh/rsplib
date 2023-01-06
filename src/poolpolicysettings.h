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
 * Copyright (C) 2003-2023 by Thomas Dreibholz
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
