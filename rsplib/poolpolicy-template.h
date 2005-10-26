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

#ifndef INTERNAL_POOLTEMPLATE
#error Do not include this file directly, use poolhandlespacemanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PoolElementNode);
struct ST_CLASS(PoolNode);

struct ST_CLASS(PoolPolicy)
{
   unsigned int Type;
   const char*  Name;

   int (*ComparisonFunction)(const struct ST_CLASS(PoolElementNode)* poolElementNode1,
                             const struct ST_CLASS(PoolElementNode)* poolElementNode2);
   size_t (*SelectionFunction)(struct ST_CLASS(PoolNode)*         poolNode,
                               struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
                               const size_t                       maxPoolElementNodes,
                               const size_t                       maxIncrement);
   void (*InitializePoolElementNodeFunction)(struct ST_CLASS(PoolElementNode)* poolElementNode);
   void (*UpdatePoolElementNodeFunction)(struct ST_CLASS(PoolElementNode)* poolElementNode);
   void (*PrepareSelectionFunction)(struct ST_CLASS(PoolNode)* poolNode);
};


extern const struct ST_CLASS(PoolPolicy) ST_CLASS(PoolPolicyArray)[];
extern const size_t ST_CLASS(PoolPolicies);


const struct ST_CLASS(PoolPolicy)* ST_CLASS(poolPolicyGetPoolPolicyByName)(const char* policyName);
const struct ST_CLASS(PoolPolicy)* ST_CLASS(poolPolicyGetPoolPolicyByType)(const unsigned int policyType);


#ifdef __cplusplus
}
#endif
