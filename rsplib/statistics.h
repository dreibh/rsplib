/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2018 by Thomas Dreibholz
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

#ifndef STATISTICS_H
#define STATISTICS_H


class Statistics
{
   public:
   Statistics();
   ~Statistics();

   /**
    * Collects one value.
    */
   void collect(double value);

   /**
   * Returns the sum of samples collected.
   */
   inline double sum() const {
     return(sumSamples);
   }

  /**
   * Returns the squared sum of the collected data.
   */
   inline double sqrSum() const {
      return(sqSumSamples);
   }

   /**
     * Returns the minimum of the samples collected.
     */
   inline double minimum() const {
      return(minSamples);
   }

   /**
     * Returns the maximum of the samples collected.
     */
   inline double maximum() const {
      return(maxSamples);
   }

   /**
     * Returns the mean of the samples collected.
    */
   inline double mean() const {
      return(numSamples ? sumSamples/numSamples : 0.0);
   }

   /**
    * Returns the standard deviation of the samples collected.
    */
   double stddev() const;

   /**
    * Returns the variance of the samples collected.
    */
   double variance() const;


   private:
   size_t numSamples;
   double sumSamples;
   double sqSumSamples;
   double minSamples;
   double maxSamples;
};


#endif
