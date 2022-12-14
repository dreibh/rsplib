/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2023 by Thomas Dreibholz
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

#include <math.h>
#include "debug.h"
#include "statistics.h"


// ###### Constructor #######################################################
Statistics::Statistics()
{
   numSamples   = 0;
   sumSamples   = 0.0;
   sqSumSamples = 0.0;
   minSamples   = 0.0;
   maxSamples   = 0.0;
}


// ###### Destructor ########################################################
Statistics::~Statistics()
{
}


// ###### Collect a valueue ###################################################
void Statistics::collect(double value)
{
   CHECK(++numSamples > 0);

   sumSamples   += value;
   sqSumSamples += value * value;

   if(numSamples > 1) {
      if(value < minSamples) {
         minSamples = value;
      }
      else if(value > maxSamples) {
         maxSamples = value;
      }
   }
   else {
       minSamples = maxSamples = value;
   }
}


// ###### Get variance ######################################################
double Statistics::variance() const
{
   if(numSamples <= 1) {
      return 0.0;
   }
   else {
      const double devsqr = (sqSumSamples - sumSamples * sumSamples / numSamples) /
                               (numSamples - 1);
      if(devsqr <= 0.0) {
         return(0.0);
      }
      else {
         return(devsqr);
      }
   }
}


// ###### Get standard deviation ############################################
double Statistics::stddev() const
{
   return(sqrt(variance()));
}
