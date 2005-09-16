#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
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




int main(int argc, char** argv)
{
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

/*
   Statistics stat;

   for(size_t i = 0;i < 1000000;i++) {
      double d = 999.0 + (random() % 3); // randomExpDouble(1000.0);

      stat.collect(d);

      // printf("%u %1.5f\n",i,d);
   }

   printf("mean=%f min=%f max=%f stddev=%f\n", stat.mean(), stat.minimum(),stat.maximum(),stat.stddev());
*/
   return 0;
}
