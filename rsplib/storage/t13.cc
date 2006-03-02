#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
   for(int i = 0;i < 1000;i++) {
      printf("i=%d\n", i);
      usleep(1000000);
   }
}
