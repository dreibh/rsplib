/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2005 by Thomas Dreibholz
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

#include "randomizer.h"


#define COMPARE_KEY_ASCENDING(a, b)  if((a) < (b)) { return(-1); } else if ((a) > (b)) { return(1); }
#define COMPARE_KEY_DESCENDING(a, b) if((a) > (b)) { return(-1); } else if ((a) < (b)) { return(1); }


/* ###### Calculate sum of 3 values and ensure datatype limit ############ */
static unsigned int ST_CLASS(getSum)(const unsigned int v1,
                                     const unsigned int v2,
                                     const unsigned int v3)
{
   const long long v1_64 = (long long)v1;
   const long long v2_64 = (long long)v2;
   const long long v3_64 = (long long)v3;
   long long v = v1_64 + v2_64 + v3_64;
   if(v < 0) {
      v = 0;
   }
   else if(v > (long long)0xffffffff) {
      v = (long long)0xffffffff;
   }
   return((unsigned int)v);
}


/* ###### Get fraction of base for difference ############################ */
static LeafLinkedTreapNodeValueType ST_CLASS(getValueFraction)(
                                       const unsigned int base,
                                       const unsigned int v1,
                                       const unsigned int v2,
                                       const unsigned int v3)
{
   const long long base_64 = (long long)base;
   const long long v1_64   = (long long)v1;
   const long long v2_64   = (long long)v2;
   const long long v3_64   = (long long)v3;
   long long v = base_64 - v1_64 - v2_64 - v3_64;
   if(v < 1) {
      /* v=0 does not make sense here: PE would never be selected. */
      v = 1;
   }
   else if(v > (long long)0xffffffff) {
      v = (long long)0xffffffff;
   }
   return((LeafLinkedTreapNodeValueType)v);
}




/*
   #######################################################################
   #### Generic Policy Functions                                      ####
   #######################################################################
*/


/* ###### Select PoolElementNodes from Storage in Order ################## */
size_t ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder)(
          struct ST_CLASS(PoolNode)*         poolNode,
          struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
          const size_t                       maxPoolElementNodes,
          const size_t                       maxIncrement)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            poolElementNodes;
   size_t                            i;
   size_t                            elementsToUpdate;


   /* Check, if resequencing is necessary. However, using 64 bit counters,
      this should (almost) never be necessary */
   CHECK(maxPoolElementNodes >= 1);
   if((PoolElementSeqNumberType)(poolNode->GlobalSeqNumber + maxPoolElementNodes) <
      poolNode->GlobalSeqNumber) {
      ST_CLASS(poolNodeResequence)(poolNode);
   }

   /* Policy-specifc pool element node updates (e.g. counter changes) */
   if(poolNode->Policy->PrepareSelectionFunction) {
      poolNode->Policy->PrepareSelectionFunction(poolNode);
   }


   poolElementNodes = 0;
   poolElementNode  = ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(poolNode);
   while((poolElementNodes < maxPoolElementNodes) && (poolElementNode != NULL)) {
      poolElementNodeArray[poolElementNodes] = poolElementNode;
      poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)(poolNode, poolElementNode);
      poolElementNodes++;
   }


   elementsToUpdate = poolElementNodes;
   if(elementsToUpdate > maxIncrement) {
      elementsToUpdate = maxIncrement;
   }
   for(i = 0;i < elementsToUpdate;i++) {
      ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(poolNode, poolElementNodeArray[i]);

      poolElementNodeArray[i]->SeqNumber = poolNode->GlobalSeqNumber++;
      poolElementNodeArray[i]->SelectionCounter++;

      /* Policy-specifc pool element node updates (e.g. counter changes) */
      if(poolNode->Policy->UpdatePoolElementNodeFunction) {
         poolNode->Policy->UpdatePoolElementNodeFunction(poolElementNodeArray[i]);
      }

      ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(poolNode, poolElementNodeArray[i]);
   }

   return(poolElementNodes);
}


/* ###### Select PoolElementNodes from Storage Randomly ################## */
size_t ST_CLASS(poolPolicySelectPoolElementNodesByValueTree)(
          struct ST_CLASS(PoolNode)*         poolNode,
          struct ST_CLASS(PoolElementNode)** poolElementNodeArray,
          const size_t                       maxPoolElementNodes,
          const size_t                       maxIncrement)
{
   LeafLinkedTreapNodeValueType maxValue;
   LeafLinkedTreapNodeValueType value;
   const size_t                 poolElements       = ST_METHOD(GetElements)(&poolNode->PoolElementSelectionStorage);
   size_t                       poolElementNodes   = 0;
   size_t                       i;


   /* Check, if resequencing is necessary. However, using 64 bit counters,
      this should (almost) never be necessary */
   CHECK(maxPoolElementNodes >= 1);
   if((PoolElementSeqNumberType)(poolNode->GlobalSeqNumber + maxPoolElementNodes) <
      poolNode->GlobalSeqNumber) {
      ST_CLASS(poolNodeResequence)(poolNode);
   }

   /* Policy-specifc pool element node updates (e.g. counter changes) */
   if(poolNode->Policy->PrepareSelectionFunction) {
      poolNode->Policy->PrepareSelectionFunction(poolNode);
   }


   for(i = 0;i < ((poolElements < maxPoolElementNodes) ? poolElements : maxPoolElementNodes);i++) {
      maxValue = ST_METHOD(GetValueSum)(&poolNode->PoolElementSelectionStorage);
      if(maxValue < 1) {
         break;
      }

      value = random64() % maxValue;
      poolElementNodeArray[poolElementNodes] =
         (struct ST_CLASS(PoolElementNode)*)ST_METHOD(GetNodeByValue)(
            &poolNode->PoolElementSelectionStorage, value);
      if(poolElementNodeArray[poolElementNodes]) {

         /* Common update functionality: SeqNumber increment and Selection Counter */
         poolElementNodeArray[poolElementNodes]->SeqNumber =
            poolNode->GlobalSeqNumber++;
         poolElementNodeArray[poolElementNodes]->SelectionCounter++;

         /* Policy-specifc pool element node updates (e.g. counter changes) */
         if(poolNode->Policy->UpdatePoolElementNodeFunction) {
            poolNode->Policy->UpdatePoolElementNodeFunction(poolElementNodeArray[poolElementNodes]);
         }

         ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(poolNode, poolElementNodeArray[poolElementNodes]);
         poolElementNodes++;
      }
      else {
         break;
      }
   }

   for(i = 0;i < poolElementNodes;i++) {
      ST_CLASS(poolNodeLinkPoolElementNodeToSelection)(poolNode, poolElementNodeArray[i]);
   }

   return(poolElementNodes);
}


/*
   #######################################################################
   #### Round Robin Policy                                            ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(roundRobinComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
}


/*
   #######################################################################
   #### Weighted Round Robin Policy                                   ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(weightedRoundRobinComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->RoundCounter, poolElementNode2->RoundCounter);
   COMPARE_KEY_ASCENDING(poolElementNode1->VirtualCounter, poolElementNode2->VirtualCounter);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
}


/* ###### Get current round counter ###################################### */
static PoolElementSeqNumberType ST_CLASS(weightedRoundRobinGetCurrentRoundCounter)(
                                   struct ST_CLASS(PoolNode)* poolNode)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode =
      ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(poolNode);
   if(poolElementNode) {
      return(poolElementNode->RoundCounter);
   }
   return(SeqNumberStart);
}


/* ###### Reset round counters to lowest possible values ################# */
static void ST_CLASS(weightedRoundRobinResetRoundCounters)(
               struct ST_CLASS(PoolNode)* poolNode)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode =
      ST_CLASS(poolNodeGetFirstPoolElementNodeFromSelection)(poolNode);

   if(poolElementNode) {
      const PoolElementSeqNumberType lowestRoundCounter = poolElementNode->RoundCounter;
      do {
         poolElementNode->RoundCounter -= lowestRoundCounter;
         poolElementNode = ST_CLASS(poolNodeGetNextPoolElementNodeFromSelection)(poolNode, poolElementNode);
      } while(poolElementNode != NULL);
   }
}


/* ###### Initialize ##################################################### */
void ST_CLASS(weightedRoundRobinInitializePoolElementNode)(
        struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->RoundCounter   = ST_CLASS(weightedRoundRobinGetCurrentRoundCounter)(
                                        poolElementNode->OwnerPoolNode);
   poolElementNode->VirtualCounter = poolElementNode->PolicySettings.Weight;
}


/* ###### Update ######################################################### */
void ST_CLASS(weightedRoundRobinUpdatePoolElementNode)(
        struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   if(poolElementNode->VirtualCounter > 1) {
      poolElementNode->VirtualCounter--;
   }
   else {
      poolElementNode->RoundCounter++;
      poolElementNode->VirtualCounter = poolElementNode->PolicySettings.Weight;
   }
}


/* ###### Prepare selection on pool ###################################### */
void ST_CLASS(weightedRoundRobinPrepareSelection)(
        struct ST_CLASS(PoolNode)* poolNode)
{
   const PoolElementSeqNumberType currentRoundCounter =
      ST_CLASS(weightedRoundRobinGetCurrentRoundCounter)(poolNode);

   if((PoolElementSeqNumberType)(currentRoundCounter + 2) < currentRoundCounter) {
      ST_CLASS(weightedRoundRobinResetRoundCounters)(poolNode);
   }
}


/*
   #######################################################################
   #### Random Policy                                                 ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(randomComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(randomUpdatePoolElementNode)(
              struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->PoolElementSelectionStorageNode.Value = 1;
}


/*
   #######################################################################
   #### Weighted Random Policy                                        ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(weightedRandomComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(weightedRandomUpdatePoolElementNode)(
              struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->PoolElementSelectionStorageNode.Value =
      poolElementNode->PolicySettings.Weight;
}


/*
   #######################################################################
   #### Least Used Policy                                             ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(leastUsedComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->PolicySettings.Load, poolElementNode2->PolicySettings.Load);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
}


/*
   #######################################################################
   #### Least Used Degradation Policy                                 ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(leastUsedDegradationComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   const unsigned int v1 = ST_CLASS(getSum)(poolElementNode1->PolicySettings.Load,
                                            poolElementNode1->Degradation, 0);
   const unsigned int v2 = ST_CLASS(getSum)(poolElementNode2->PolicySettings.Load,
                                            poolElementNode2->Degradation, 0);
   COMPARE_KEY_ASCENDING(v1, v2);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(leastUsedDegradationUpdatePoolElementNode)(
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->Degradation =
      ST_CLASS(getSum)(poolElementNode->Degradation,
                       poolElementNode->PolicySettings.LoadDegradation,
                       0);
}


/*
   #######################################################################
   #### Priority Least Used Policy                                    ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(priorityLeastUsedComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   const unsigned int v1 = ST_CLASS(getSum)(poolElementNode1->PolicySettings.Load,
                                            poolElementNode1->PolicySettings.LoadDegradation, 0);
   const unsigned int v2 = ST_CLASS(getSum)(poolElementNode2->PolicySettings.Load,
                                            poolElementNode2->PolicySettings.LoadDegradation, 0);
   COMPARE_KEY_ASCENDING(v1, v2);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
}


/*
   #######################################################################
   #### Priority Least Used Degradation Policy                        ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(priorityLeastUsedDegradationComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   const unsigned int v1 = ST_CLASS(getSum)(poolElementNode1->PolicySettings.Load,
                                            poolElementNode1->PolicySettings.LoadDegradation,
                                            poolElementNode1->Degradation);
   const unsigned int v2 = ST_CLASS(getSum)(poolElementNode2->PolicySettings.Load,
                                            poolElementNode2->PolicySettings.LoadDegradation,
                                            poolElementNode2->Degradation);
   COMPARE_KEY_ASCENDING(v1, v2);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(priorityLeastUsedDegradationUpdatePoolElementNode)(
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->Degradation =
      ST_CLASS(getSum)(poolElementNode->Degradation,
                       poolElementNode->PolicySettings.LoadDegradation,
                       0);
}


/*
   #######################################################################
   #### Randomized Least Used Policy                                  ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(randomizedLeastUsedComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(randomizedLeastUsedUpdatePoolElementNode)(
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->PoolElementSelectionStorageNode.Value =
      ST_CLASS(getValueFraction)(PEPS_MAX_LOAD,
                                 poolElementNode->PolicySettings.Load,
                                 0, 0);
}


/*
   #######################################################################
   #### Randomized Least Used Degradation Policy                      ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(randomizedLeastUsedDegradationComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(randomizedLeastUsedDegradationUpdatePoolElementNode)(
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->PoolElementSelectionStorageNode.Value =
      ST_CLASS(getValueFraction)(PEPS_MAX_LOAD,
                                 poolElementNode->PolicySettings.Load,
                                 poolElementNode->Degradation, 0);
}


/*
   #######################################################################
   #### Randomized Priority Least Used Policy                         ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(randomizedPriorityLeastUsedComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(randomizedPriorityLeastUsedUpdatePoolElementNode)(
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->PoolElementSelectionStorageNode.Value =
      ST_CLASS(getValueFraction)(PEPS_MAX_LOAD,
                                 poolElementNode->PolicySettings.Load,
                                 poolElementNode->PolicySettings.LoadDegradation,
                                 0);
}


/*
   #######################################################################
   #### Randomized Priority Least Used Degradation Policy             ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(randomizedPriorityLeastUsedDegradationComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(randomizedPriorityLeastUsedDegradationUpdatePoolElementNode)(
               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   poolElementNode->PoolElementSelectionStorageNode.Value =
      ST_CLASS(getValueFraction)(PEPS_MAX_LOAD,
                                 poolElementNode->PolicySettings.Load,
                                 poolElementNode->PolicySettings.LoadDegradation,
                                 poolElementNode->Degradation);
}


const struct ST_CLASS(PoolPolicy) ST_CLASS(PoolPolicyArray)[] =
{
   {
      PPT_ROUNDROBIN, "RoundRobin",
      &ST_CLASS(roundRobinComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_WEIGHTED_ROUNDROBIN, "WeightedRoundRobin",
      &ST_CLASS(weightedRoundRobinComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      &ST_CLASS(weightedRoundRobinInitializePoolElementNode),
      &ST_CLASS(weightedRoundRobinUpdatePoolElementNode),
      &ST_CLASS(weightedRoundRobinPrepareSelection)
   },
   {
      PPT_RANDOM, "Random",
      &ST_CLASS(randomComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomUpdatePoolElementNode),
      NULL
   },
   {
      PPT_WEIGHTED_RANDOM, "WeightedRandom",
      &ST_CLASS(weightedRandomComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(weightedRandomUpdatePoolElementNode),
      NULL
   },

   {
      PPT_LEASTUSED, "LeastUsed",
      &ST_CLASS(leastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_LEASTUSED_DEGRADATION, "LeastUsedDegradation",
      &ST_CLASS(leastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      &ST_CLASS(leastUsedDegradationUpdatePoolElementNode),
      NULL
   },
   {
      PPT_PRIORITY_LEASTUSED, "PriorityLeastUsed",
      &ST_CLASS(priorityLeastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_PRIORITY_LEASTUSED_DEGRADATION, "PriorityLeastUsedDegradation",
      &ST_CLASS(priorityLeastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      &ST_CLASS(priorityLeastUsedDegradationUpdatePoolElementNode),
      NULL
   },

   {
      PPT_RANDOMIZED_LEASTUSED, "RandomizedLeastUsed",
      &ST_CLASS(randomizedLeastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedLeastUsedUpdatePoolElementNode),
      NULL
   },
   {
      PPT_RANDOMIZED_LEASTUSED_DEGRADATION, "RandomizedLeastUsedDegradation",
      &ST_CLASS(randomizedLeastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedLeastUsedDegradationUpdatePoolElementNode),
      NULL
   },
   {
      PPT_RANDOMIZED_PRIORITY_LEASTUSED, "RandomizedPriorityLeastUsed",
      &ST_CLASS(randomizedPriorityLeastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedPriorityLeastUsedUpdatePoolElementNode),
      NULL
   },
   {
      PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION, "RandomizedPriorityLeastUsedDegradation",
      &ST_CLASS(randomizedPriorityLeastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedPriorityLeastUsedDegradationUpdatePoolElementNode),
      NULL
   }
};

const size_t ST_CLASS(PoolPolicies) = sizeof(ST_CLASS(PoolPolicyArray)) /
                                         sizeof(struct ST_CLASS(PoolPolicy));




/* ###### Get policy type for policy name ################################ */
const struct ST_CLASS(PoolPolicy)* ST_CLASS(poolPolicyGetPoolPolicyByName)(const char* policyName)
{
   size_t i;
   for(i = 0;i < ST_CLASS(PoolPolicies);i++) {
      if(strcmp(ST_CLASS(PoolPolicyArray)[i].Name, policyName) == 0) {
         return(&ST_CLASS(PoolPolicyArray)[i]);
      }
   }
   return(NULL);
}


/* ###### Get policy name for policy type ################################ */
const struct ST_CLASS(PoolPolicy)* ST_CLASS(poolPolicyGetPoolPolicyByType)(const unsigned int policyType)
{
   size_t i;
   for(i = 0;i < ST_CLASS(PoolPolicies);i++) {
      if(ST_CLASS(PoolPolicyArray)[i].Type == policyType) {
         return(&ST_CLASS(PoolPolicyArray)[i]);
      }
   }
   return(NULL);
}
