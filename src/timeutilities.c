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
 * Copyright (C) 2003-2025 by Thomas Dreibholz
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

#include "timeutilities.h"
#include <sys/time.h>
#include <time.h>


/* ###### Get current timer ############################################## */
unsigned long long getMicroTime()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return(((unsigned long long)tv.tv_sec * (unsigned long long)1000000) +
         (unsigned long long)tv.tv_usec);
}


/* ###### Print time stamp ############################################### */
void printTimeStamp(FILE* fd)
{
   char                     str[64];
   const unsigned long long microTime = getMicroTime();
   const time_t             timeStamp = microTime / 1000000;
   const struct tm*         timeptr   = localtime(&timeStamp);

   strftime((char*)&str, sizeof(str), "%d-%b-%Y %H:%M:%S", timeptr);
   fputs(str, fd);
   fprintf(fd, ".%04d: ", (unsigned int)(microTime % 1000000) / 100);
}


/* ###### Initialize weighted statistics value ########################### */
void initWeightedStatValue(struct WeightedStatValue* value,
                           const unsigned long long  now)
{
   value->Value           = 0.0;
   value->Cumulation      = 0.0;
   value->StartTimeStamp  = now;
   value->UpdateTimeStamp = now;
}


/* ###### Update weighted statistics value ############################### */
void updateWeightedStatValue(struct WeightedStatValue* value,
                             const unsigned long long  now,
                             const double              v)
{
   const unsigned long long elapsedSinceLastUpdate = now - value->UpdateTimeStamp;
   value->Cumulation     += value->Value * (elapsedSinceLastUpdate / 1000000.0);
   value->Value           = v;
   value->UpdateTimeStamp = now;
}


/* ###### Get weighted statistics value ################################## */
double averageWeightedStatValue(struct WeightedStatValue* value,
                                const unsigned long long  now)
{
   const unsigned long long elapsedSinceStart = now - value->StartTimeStamp;
   updateWeightedStatValue(value, now, value->Value);
   return(value->Cumulation / (elapsedSinceStart / 1000000.0));
}
