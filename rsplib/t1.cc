#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "netutilities.h"
#include "registrar.h"


struct WeightedStatValue
{
   double             Value;
   double             Cumulation;
   unsigned long long StartTimeStamp;
   unsigned long long UpdateTimeStamp;
};


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
   printf("C=%1.6f  v=%1.6f\n",value->Cumulation,v);
}


/* ###### Get weighted statistics value ################################## */
double getWeightedStatValue(struct WeightedStatValue* value,
                            const unsigned long long  now)
{
   const unsigned long long elapsedSinceStart = now - value->StartTimeStamp;
   updateWeightedStatValue(value, now, value->Value);
   return(value->Cumulation / (elapsedSinceStart / 1000000.0));
}


int main(int argc, char** argv)
{
   WeightedStatValue v;
   initWeightedStatValue(&v,1000000);
   updateWeightedStatValue(&v, 1000000,10);
   updateWeightedStatValue(&v, 2000000,5);
   updateWeightedStatValue(&v, 3000000,10);
   updateWeightedStatValue(&v, 4000000,5);
   printf("r1=%1.6f\n", getWeightedStatValue(&v, 5000000));
}
