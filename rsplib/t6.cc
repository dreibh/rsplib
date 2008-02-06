#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char** argv)
{
   printf("sizeof(unsigned int)       = %d\n", sizeof(unsigned int));
   printf("sizeof(unsigned long)      = %d\n", sizeof(unsigned long));
   printf("sizeof(unsigned long long) = %d\n", sizeof(unsigned long long));
   printf("sizeof(void*)              = %d\n", sizeof(void*));
   return 0;
}
