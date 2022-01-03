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
 * Copyright (C) 2003-2022 by Thomas Dreibholz
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
   else if(v > (long long)PPV_MAX_LOAD) {
      v = (long long)PPV_MAX_LOAD;
   }
   return((unsigned int)v);
}


/* ###### Get fraction of base for difference ############################ */
static unsigned long long ST_CLASS(getValueFraction)(
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
   else if(v > (long long)PPV_MAX_WEIGHT) {
      v = (long long)PPV_MAX_WEIGHT;
   }
   return((unsigned long long)v);
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
          size_t                             maxIncrement)
{
   struct ST_CLASS(PoolElementNode)* poolElementNode;
   size_t                            poolElementNodes;
   size_t                            i;
   size_t                            elementsToUpdate;

   /* Set maxIncrement to default, if maxIncrement == 0. */
   if(maxIncrement == 0) {
      maxIncrement = poolNode->Policy->DefaultMaxIncrement;
   }

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
          size_t                             maxIncrement)
{
   unsigned long long maxValue;
   unsigned long long value;
   const size_t       poolElements     = ST_METHOD(GetElements)(&poolNode->PoolElementSelectionStorage);
   size_t             poolElementNodes = 0;
   size_t             i;

   /* Set maxIncrement to default, if maxIncrement == 0. */
   if(maxIncrement == 0) {
      maxIncrement = poolNode->Policy->DefaultMaxIncrement;
   }

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

         /* Update PE entries with respect to maxIncrement setting. */
         if(poolElementNodes < maxIncrement) {
            /* Policy-specifc pool element node updates (e.g. counter changes) */
            if(poolNode->Policy->UpdatePoolElementNodeFunction) {
               poolNode->Policy->UpdatePoolElementNodeFunction(poolElementNodeArray[poolElementNodes]);
            }
         }

         /* Unlinking *must* be done for all PEs
               -> otherwise, multiple selections of the same PE possible! */
         ST_CLASS(poolNodeUnlinkPoolElementNodeFromSelection)(poolNode, poolElementNodeArray[poolElementNodes]);
         poolElementNodes++;
      }
      else {
         break;
      }
   }

   /* Re-linking of all previously unlinked nodes */
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
   #### Weighted Random with Distance Penalty Factor Policy           ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(weightedRandomDPFComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_ASCENDING(poolElementNode1->Identifier, poolElementNode2->Identifier);
   return(0);
}


/* ###### Update ######################################################### */
static void ST_CLASS(weightedRandomDPFUpdatePoolElementNode)(
              struct ST_CLASS(PoolElementNode)* poolElementNode)
{
   const double dpf = (double)poolElementNode->PolicySettings.Distance * ((double)poolElementNode->PolicySettings.WeightDPF / (double)PPV_MAX_WEIGHTDPF);
   long long value  = (long long)poolElementNode->PolicySettings.Weight -
                         (long long)rint((double)poolElementNode->PolicySettings.Weight * dpf);
   if(value < 0) {
      value = 1;
   }
   else if(value > (long long)PPV_MAX_WEIGHT) {
      value = (long long)PPV_MAX_WEIGHT;
   }

   poolElementNode->PoolElementSelectionStorageNode.Value = (unsigned int)value;
}


/*
   #######################################################################
   #### Priority Policy                                               ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(priorityComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   COMPARE_KEY_DESCENDING(poolElementNode1->PolicySettings.Weight, poolElementNode2->PolicySettings.Weight);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
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
   #### Least Used with Distance Penalty Factor Policy                ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(leastUsedDPFComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   const double dpf1 = (double)poolElementNode1->PolicySettings.Distance * ((double)poolElementNode1->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF);
   const double dpf2 = (double)poolElementNode2->PolicySettings.Distance * ((double)poolElementNode2->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF);

   unsigned long long v1 = (unsigned long long)rint((double)poolElementNode1->PolicySettings.Load + (dpf1 * (double)PPV_MAX_LOAD));
   if(v1 > (long long)PPV_MAX_LOAD) {
      v1 = (long long)PPV_MAX_LOAD;
   }
   unsigned long long v2 = (unsigned long long)rint((double)poolElementNode2->PolicySettings.Load + (dpf2 * (double)PPV_MAX_LOAD));
   if(v2 > (long long)PPV_MAX_LOAD) {
      v2 = (long long)PPV_MAX_LOAD;
   }

/*
   printf("dpf1=%1.6lf l1=$%x ldpf1=%1.8lf d1=%3u v1=$%x ; ",
          dpf1,
          poolElementNode1->PolicySettings.Load,
          ((double)poolElementNode1->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF),
          poolElementNode1->PolicySettings.Distance,
          v1);
   printf("dpf2=%1.6lf l2=$%x ldpf2=%1.8lf d2=%3u v2=$%x\n",
          dpf2,
          poolElementNode2->PolicySettings.Load,
          ((double)poolElementNode2->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF),
          poolElementNode2->PolicySettings.Distance,
          v2);
*/

   COMPARE_KEY_ASCENDING(v1, v2);
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
   #### Least Used with Degradation & Distance Penalty Factor Policy  ####
   #######################################################################
*/

/* ###### Sorting Order ################################################## */
static int ST_CLASS(leastUsedDegradationDPFComparison)(
   const struct ST_CLASS(PoolElementNode)* poolElementNode1,
   const struct ST_CLASS(PoolElementNode)* poolElementNode2)
{
   const double dpf1 = (double)poolElementNode1->PolicySettings.Distance * ((double)poolElementNode1->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF);
   const double dpf2 = (double)poolElementNode2->PolicySettings.Distance * ((double)poolElementNode2->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF);

   unsigned long long v1 = (unsigned long long)rint(
      (double)poolElementNode1->PolicySettings.Load +
      (double)poolElementNode1->Degradation +
      (dpf1 * (double)PPV_MAX_LOADDPF));
   if(v1 > (long long)PPV_MAX_LOAD) {
      v1 = (long long)PPV_MAX_LOAD;
   }
   unsigned long long v2 = (unsigned long long)rint(
      (double)poolElementNode2->PolicySettings.Load +
      (double)poolElementNode2->Degradation +
      (dpf2 * (double)PPV_MAX_LOAD));
   if(v2 > (long long)PPV_MAX_LOAD) {
      v2 = (long long)PPV_MAX_LOAD;
   }

/*
   printf("dpf1=%1.6lf deg1=%1.3f%% l1=$%x ldpf1=%1.8lf d1=%3u v1=$%llx ; ",
          dpf1,
          poolElementNode1->Degradation * 100.0 / (double)PPV_MAX_LOADDEGRADATION,
          poolElementNode1->PolicySettings.Load,
          ((double)poolElementNode1->PolicySettings.LoadDPF / (double)(long long)PPV_MAX_LOAD_DPF),
          poolElementNode1->PolicySettings.Distance,
          v1);
   printf("dpf2=%1.6lf deg1=%1.3f%% l2=$%x ldpf2=%1.8lf d2=%3u v2=$%llx\n",
          dpf2,
          poolElementNode2->Degradation * 100.0 / (double)PPV_MAX_LOAD_DEGRADATION,
          poolElementNode2->PolicySettings.Load,
          ((double)poolElementNode2->PolicySettings.LoadDPF / (double)PPV_MAX_LOADDPF),
          poolElementNode2->PolicySettings.Distance,
          v2);
*/

   COMPARE_KEY_ASCENDING(v1, v2);
   COMPARE_KEY_ASCENDING(poolElementNode1->SeqNumber, poolElementNode2->SeqNumber);
   return(0);
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
      ST_CLASS(getValueFraction)(PPV_MAX_LOAD,
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
      ST_CLASS(getValueFraction)(PPV_MAX_LOAD,
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
      ST_CLASS(getValueFraction)(PPV_MAX_LOAD,
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
      ST_CLASS(getValueFraction)(PPV_MAX_LOAD,
                                 poolElementNode->PolicySettings.Load,
                                 poolElementNode->PolicySettings.LoadDegradation,
                                 poolElementNode->Degradation);
}


const struct ST_CLASS(PoolPolicy) ST_CLASS(PoolPolicyArray)[] =
{
   {
      PPT_ROUNDROBIN, "RoundRobin",
      1,
      &ST_CLASS(roundRobinComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_WEIGHTED_ROUNDROBIN, "WeightedRoundRobin",
      1,
      &ST_CLASS(weightedRoundRobinComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      &ST_CLASS(weightedRoundRobinInitializePoolElementNode),
      &ST_CLASS(weightedRoundRobinUpdatePoolElementNode),
      &ST_CLASS(weightedRoundRobinPrepareSelection)
   },
   {
      PPT_RANDOM, "Random",
      0,
      &ST_CLASS(randomComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomUpdatePoolElementNode),
      NULL
   },
   {
      PPT_WEIGHTED_RANDOM, "WeightedRandom",
      0,
      &ST_CLASS(weightedRandomComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(weightedRandomUpdatePoolElementNode),
      NULL
   },
   {
      PPT_WEIGHTED_RANDOM_DPF, "WeightedRandomDPF",
      0,
      &ST_CLASS(weightedRandomDPFComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(weightedRandomDPFUpdatePoolElementNode),
      NULL
   },
   {
      PPT_PRIORITY, "Priority",
      1,
      &ST_CLASS(priorityComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },

   {
      PPT_LEASTUSED, "LeastUsed",
      1,
      &ST_CLASS(leastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_LEASTUSED_DPF, "LeastUsedDPF",
      1,
      &ST_CLASS(leastUsedDPFComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_LEASTUSED_DEGRADATION, "LeastUsedDegradation",
      1,
      &ST_CLASS(leastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      &ST_CLASS(leastUsedDegradationUpdatePoolElementNode),
      NULL
   },
   {
      PPT_LEASTUSED_DEGRADATION_DPF, "LeastUsedDegradationDPF",
      1,
      &ST_CLASS(leastUsedDegradationDPFComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      &ST_CLASS(leastUsedDegradationUpdatePoolElementNode),
      NULL
   },
   {
      PPT_PRIORITY_LEASTUSED, "PriorityLeastUsed",
      1,
      &ST_CLASS(priorityLeastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      NULL,
      NULL
   },
   {
      PPT_PRIORITY_LEASTUSED_DEGRADATION, "PriorityLeastUsedDegradation",
      1,
      &ST_CLASS(priorityLeastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesBySortingOrder),
      NULL,
      &ST_CLASS(priorityLeastUsedDegradationUpdatePoolElementNode),
      NULL
   },

   {
      PPT_RANDOMIZED_LEASTUSED, "RandomizedLeastUsed",
      1,
      &ST_CLASS(randomizedLeastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedLeastUsedUpdatePoolElementNode),
      NULL
   },
   {
      PPT_RANDOMIZED_LEASTUSED_DEGRADATION, "RandomizedLeastUsedDegradation",
      1,
      &ST_CLASS(randomizedLeastUsedDegradationComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedLeastUsedDegradationUpdatePoolElementNode),
      NULL
   },
   {
      PPT_RANDOMIZED_PRIORITY_LEASTUSED, "RandomizedPriorityLeastUsed",
      1,
      &ST_CLASS(randomizedPriorityLeastUsedComparison),
      &ST_CLASS(poolPolicySelectPoolElementNodesByValueTree),
      NULL,
      &ST_CLASS(randomizedPriorityLeastUsedUpdatePoolElementNode),
      NULL
   },
   {
      PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION, "RandomizedPriorityLeastUsedDegradation",
      1,
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
