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
