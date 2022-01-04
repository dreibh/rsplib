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

#ifndef TIMEUTILITIES_H
#define TIMEUTILITIES_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
  * Get current time: Microseconds since 01 January, 1970.
  *
  * @return Current time.
  */
unsigned long long getMicroTime();

/**
  * Print time stamp.
  *
  * @param fd File to print timestamp to (e.g. stdout, stderr, ...).
  */
void printTimeStamp(FILE* fd);


struct WeightedStatValue
{
   double             Value;
   double             Cumulation;
   unsigned long long StartTimeStamp;
   unsigned long long UpdateTimeStamp;
};


/**
  * Initialize WeightedStatValue object.
  *
  * @param value WeightedStatValue.
  * @param now Current time stamp.
  */
void initWeightedStatValue(struct WeightedStatValue* value,
                           const unsigned long long  now);


/**
  * Update WeightedStatValue object by new value.
  *
  * @param value WeightedStatValue.
  * @param now Current time stamp.
  * @param v New value.
  */
void updateWeightedStatValue(struct WeightedStatValue* value,
                             const unsigned long long  now,
                             const double              v);


/**
  * Compute average from WeightedStatValue.
  *
  * @param value WeightedStatValue.
  * @param now Current time stamp.
  */
double averageWeightedStatValue(struct WeightedStatValue* value,
                                const unsigned long long  now);



#ifdef __cplusplus
}
#endif

#endif

