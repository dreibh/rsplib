#include "tdtypes.h"
#include "debug.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


size_t Sessions = 0;
unsigned int LoadSum = 0;

class X
{
   public:
   X();
   double getLoad() const;
   void setLoad(double load);

   private:
   unsigned int Load;
};


X::X() {
   Load = 0;
}

double X::getLoad() const
{
   return((double)Load / (double)0xffffff);
}


void X::setLoad(double load)
{
   const unsigned int newLoad = (unsigned int)rint(load * (double)0xffffff);
   if((load < 0.0) || (load > 1.0)) {
      fputs("ERROR: Invalid load setting!\n", stderr);
      return;
   }
   CHECK(LoadSum >= Load);

   if(LoadSum - Load + newLoad > 0xffffff) {
      fputs("ERROR: Something is wrong with load settings. Total load would exceed 100%%!\n", stderr);
      return;
   }

   LoadSum -= Load;
   Load     = newLoad;
   LoadSum += Load;
}


double getTotalLoad()
{
   if(Sessions > 0) {
      return(LoadSum / (double)0xffffff);
   }
   return(0.0);
}


int main(int argc, char** argv)
{
   X x1;
   X x2;
   X x3;
   Sessions = 3;
   size_t MaxSessions = 3;

   x1.setLoad(1.0/MaxSessions);
   x3.setLoad(1.0/MaxSessions);
   x2.setLoad(0.5/MaxSessions);

   printf("Total Load = %1.3f\n", getTotalLoad());


   x2.setLoad(1.0/MaxSessions);
   printf("Total Load = %1.3f\n", getTotalLoad());

   x2.setLoad(0.0);
   printf("Total Load = %1.3f\n", getTotalLoad());
}
