/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2005 by Thomas Dreibholz
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

#define INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolhandlespacemanagement.h"


/* ###### Get policy Type of policy name ################################### */
unsigned int poolPolicyGetPoolPolicyTypeByName(const char* policyName)
{
   size_t i;
   for(i = 0;i < TMPL_CLASS(PoolPolicies,LeafLinkedRedBlackTree);i++) {
      if(strcmp(TMPL_CLASS(PoolPolicyArray,LeafLinkedRedBlackTree)[i].Name, policyName) == 0) {
         return(TMPL_CLASS(PoolPolicyArray,LeafLinkedRedBlackTree)[i].Type);
      }
   }
   return(PPT_UNDEFINED);
}


/* ###### Get policy name of policy Type ################################### */
const char* poolPolicyGetPoolPolicyNameByType(const unsigned int policyType)
{
   size_t i;
   for(i = 0;i < TMPL_CLASS(PoolPolicies,LeafLinkedRedBlackTree);i++) {
      if(TMPL_CLASS(PoolPolicyArray,LeafLinkedRedBlackTree)[i].Type == policyType) {
         return(TMPL_CLASS(PoolPolicyArray,LeafLinkedRedBlackTree)[i].Name);
      }
   }
   return(NULL);
}
