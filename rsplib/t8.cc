#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sys/poll.h>
#include "ext_socket.h"
#include "randomizer.h"
#include "statistics.h"
#include "debug.h"



struct IdentifierBitmap
{
   size_t Entries;
   size_t Available;

   size_t Slots;
   size_t Bitmap[0];
};

#define IdentifierBitmapSlotsize (sizeof(size_t) * 8)


/* ###### Constructor #################################################### */
struct IdentifierBitmap* identifierBitmapNew(const size_t entries)
{
   const size_t slots = (entries + (IdentifierBitmapSlotsize - (entries % IdentifierBitmapSlotsize))) /
                           IdentifierBitmapSlotsize;
   struct IdentifierBitmap* identifierBitmap = (struct IdentifierBitmap*)malloc(sizeof(IdentifierBitmap) + (slots + 1) * sizeof(size_t));
   if(identifierBitmap) {
      memset(&identifierBitmap->Bitmap, 0, (slots + 1) * sizeof(size_t));
      identifierBitmap->Entries   = entries;
      identifierBitmap->Available = entries;
      identifierBitmap->Slots     = slots;
   }
   return(identifierBitmap);
}


/* ###### Destructor ##################################################### */
void identifierBitmapDelete(struct IdentifierBitmap* identifierBitmap)
{
   identifierBitmap->Entries = 0;
   free(identifierBitmap);
}


/* ###### Allocate ID #################################################### */
int identifierBitmapAllocateID(struct IdentifierBitmap* identifierBitmap)
{
   size_t i, j;
   int    id = -1;

   if(identifierBitmap->Available > 0) {
      i = 0;
      while(identifierBitmap->Bitmap[i] == ~((size_t)0)) {
         i++;
      }
      id = i * IdentifierBitmapSlotsize;

      j = 0;
      while((j < IdentifierBitmapSlotsize) &&
            (id < (int)identifierBitmap->Entries) &&
            (identifierBitmap->Bitmap[i] & (1 << j))) {
         j++;
         id++;
      }
      CHECK(id < (int)identifierBitmap->Entries);

      identifierBitmap->Bitmap[i] |= (1 << j);
      identifierBitmap->Available--;
   }

   return(id);
}


/* ###### Allocate specific ID ########################################### */
int identifierBitmapAllocateSpecificID(struct IdentifierBitmap* identifierBitmap,
                                       const int                id)
{
   size_t i, j;

   CHECK((id >= 0) && (id < (int)identifierBitmap->Entries));
   i = id / IdentifierBitmapSlotsize;
   j = id % IdentifierBitmapSlotsize;
   if(identifierBitmap->Bitmap[i] & (1 << j)) {
      return(-1);
   }
   identifierBitmap->Bitmap[i] |= (1 << j);
   identifierBitmap->Available--;
   return(id);
}


/* ###### Free ID ######################################################## */
void identifierBitmapFreeID(struct IdentifierBitmap* identifierBitmap, const int id)
{
   size_t i, j;

   CHECK((id >= 0) && (id < (int)identifierBitmap->Entries));
   i = id / IdentifierBitmapSlotsize;
   j = id % IdentifierBitmapSlotsize;
   CHECK(identifierBitmap->Bitmap[i] & (1 << j));
   identifierBitmap->Bitmap[i] &= ~(1 << j);
   identifierBitmap->Available++;
}










int RsplibPipe[2];

void* test(void* arg)
{
   char buffer[1];
   ssize_t rd;

   puts("---------------------");
   rd = ext_read(RsplibPipe[0], (char*)&buffer, sizeof(buffer));
   if(rd > 0) {
      puts("HAS-DATA-0!");
   } else printf("no-data-0   r=%d  e=%d\n",rd,errno);

   rd = ext_recv(RsplibPipe[0], (char*)&buffer, sizeof(buffer), 0);
   if(rd > 0) {
      puts("HAS-DATA-1!");
   } else printf("no-data-1   r=%d  e=%d <-- Sollte Fehler sein (kein Socket)\n",rd,errno);

   puts("---------------------");
   fd_set a;
   FD_ZERO(&a);
   FD_SET(RsplibPipe[0], &a);
   timeval to;
   to.tv_sec=0;
   to.tv_usec=0;
   int r = ext_select(RsplibPipe[0]+1,&a,NULL,NULL,&to);
   if((r>0)&&(FD_ISSET(RsplibPipe[0],&a))) puts("HAS-DATA-2!"); else puts("no-data-2");

   puts("---------------------");
   struct pollfd u0;
   u0.fd=RsplibPipe[0];
   u0.events=POLLIN;
   u0.revents=0;
   int r2=ext_poll(&u0, 1, 0);
   if((r2>0)&&(u0.revents & POLLIN)) puts("HAS-DATA-3!"); else printf("no-data-3   rev=%x\n",u0.revents);

   return(NULL);
}


int main(int argc, char** argv)
{
#if 0
   IdentifierBitmap* ib = identifierBitmapNew(1024);
   size_t i;

   CHECK(identifierBitmapAllocateSpecificID(ib, 0) == 0);
   for(i = 0;i < 17;i++) {
      printf("#%d  ->  id=%d\n", i, identifierBitmapAllocateID(ib));
   }

   puts("----");
   identifierBitmapFreeID(ib, 11);
   identifierBitmapFreeID(ib, 13);
   identifierBitmapFreeID(ib, 7);
   identifierBitmapFreeID(ib, 3);
   puts("----");

   for(i = 0;i < 6;i++) {
      printf("#%d  ->  id=%d\n", i, identifierBitmapAllocateID(ib));
   }


   identifierBitmapDelete(ib);
#endif
/*
   Statistics stat;

   for(size_t i = 0;i < 1000000;i++) {
      double d = 999.0 + (random() % 3); // randomExpDouble(1000.0);

      stat.collect(d);

      // printf("%u %1.5f\n",i,d);
   }

   printf("mean=%f min=%f max=%f stddev=%f\n", stat.mean(), stat.minimum(),stat.maximum(),stat.stddev());
*/

   ext_pipe((int*)&RsplibPipe);
   ext_write(RsplibPipe[1], "xx", 2);

   pthread_t thr;
   pthread_create(&thr, NULL, test, NULL);


   for(;;) {
      usleep(1000000);
   }

   return 0;
}
