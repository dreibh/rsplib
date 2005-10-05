#include "tdtypes.h"
#include "debug.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


size_t Sessions = 0;
unsigned long long LoadSum = 0;

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
   return((double)Load / (double)0xffffffff);
}


void X::setLoad(double load)
{
   if((load < 0.0) || (load > 1.0)) {
      fputs("ERROR: Invalid load setting!\n", stderr);
      return;
   }

   CHECK(LoadSum >= Load);
   LoadSum -= Load;
   Load     = (unsigned int)rint(load * (double)0xffffffff);
   LoadSum += Load;
}


double getTotalLoad()
{
   if(Sessions > 0) {
      return(LoadSum / (Sessions * (double)0xffffffff));
   }
   return(0.0);
}


int main(int argc, char** argv)
{
   X x1;
   X x2;
   X x3;
   Sessions = 3;

   x1.setLoad(1.0);
   x3.setLoad(1.0);
   x2.setLoad(.5);

   printf("Total Load = %1.3f\n", getTotalLoad());


   x2.setLoad(1.0);
   printf("Total Load = %1.3f\n", getTotalLoad());

   x2.setLoad(0.0);
   printf("Total Load = %1.3f\n", getTotalLoad());
}
