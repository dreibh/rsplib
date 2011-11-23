/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2012 by Thomas Dreibholz
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
 * Contact: dreibh@iem.uni-due.de
 */

#define INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolhandlespacemanagement.h"


/* ###### Get policy Type of policy name ################################### */
unsigned int poolPolicyGetPoolPolicyTypeByName(const char* policyName)
{
   size_t i;
   for(i = 0;i < TMPL_CLASS(PoolPolicies,SimpleRedBlackTree);i++) {
      if(strcmp(TMPL_CLASS(PoolPolicyArray,SimpleRedBlackTree)[i].Name, policyName) == 0) {
         return(TMPL_CLASS(PoolPolicyArray,SimpleRedBlackTree)[i].Type);
      }
   }
   return(PPT_UNDEFINED);
}


/* ###### Get policy name of policy Type ################################### */
const char* poolPolicyGetPoolPolicyNameByType(const unsigned int policyType)
{
   size_t i;
   for(i = 0;i < TMPL_CLASS(PoolPolicies,SimpleRedBlackTree);i++) {
      if(TMPL_CLASS(PoolPolicyArray,SimpleRedBlackTree)[i].Type == policyType) {
         return(TMPL_CLASS(PoolPolicyArray,SimpleRedBlackTree)[i].Name);
      }
   }
   return(NULL);
}
