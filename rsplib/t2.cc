#include "scriptingservice.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"
#include "loglevel.h"

#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <set>
#include <string>


class EnvironmentCache : public TDMutex
{
   public:
   EnvironmentCache();
   ~EnvironmentCache();

   bool initialize(const char* directory);

   void print();

   bool isInCache(const uint8_t* hash);

   bool storeInCache(const uint8_t* hash, const char* fileName);
   bool copyFromCache(const uint8_t* hash, const char* fileName);

   private:
   static void printHash(FILE* fh, const uint8_t* hash);
   static unsigned long long copyFile(const char*    source,
                                      const char*    destination,
                                      const uint8_t* hash);

   struct CacheEntry {
      uint8_t            Hash[SE_HASH_SIZE];
      unsigned long long Size;
      unsigned long long LastTimeUsed;
      std::string        FileName;
   };
   std::set<CacheEntry*> Cache;
   std::string           CacheDirectory;
};


EnvironmentCache::EnvironmentCache()
{
}


EnvironmentCache::~EnvironmentCache()
{
}


bool EnvironmentCache::initialize(const char* directory)
{
   CacheDirectory = "/tmp";
   return(true);
}


void EnvironmentCache::printHash(FILE* fh, const uint8_t* hash)
{
   for(size_t i = 0; i < SE_HASH_SIZE; i++) {
      fprintf(fh, "%02x", (unsigned int)hash[i]);
   }
}


unsigned long long EnvironmentCache::copyFile(const char*    source,
                                              const char*    destination,
                                              const uint8_t* hash)
{
   unsigned long long size = 0;
   sha1_ctx           context;
   uint8_t*           newHash[SE_HASH_SIZE];

   if(hash) {
      sha1_init(&context);
   }

   FILE* input = fopen(source, "r");
   if(input != NULL) {
      FILE* output = fopen(destination, "w");
      if(output != NULL) {

         char buffer[65536];
         while(!feof(input)) {
            const size_t count = fread(&buffer, 1, sizeof(buffer), input);
            if(count > 0) {
               if(fwrite(&buffer, 1, count, output) != count) {
                  size = 0;
                  break;
               }
               if(hash) {
                  sha1_update(&context, (uint8_t*)&buffer, count);
               }
               size += (unsigned long long)count;
            }
         }

         if(hash) {
            sha1_final(&context, (uint8_t*)&newHash);
            if(memcmp(hash, &newHash, SE_HASH_SIZE) != 0) {
               puts("CACHE ERROR!");
               printHash(stdout, (uint8_t*)hash); puts("");
               printHash(stdout, (uint8_t*)&newHash); puts("");
               size = 0;
            }
         }
         if(fclose(output) != 0) {
            size = 0;
         }
      }
      fclose(input);
   }

   printf("S=%llu\n",size);
   return(size);
}


void EnvironmentCache::print()
{
   for(std::set<CacheEntry*>::const_iterator iterator = Cache.begin();
       iterator != Cache.end(); iterator++) {
      const CacheEntry* entry = *iterator;
      printHash(stdout, (uint8_t*)&entry->Hash);
      fprintf(stdout, "\t%s\t%llu\t%llu\n",
              entry->FileName.c_str(),
              entry->Size, entry->LastTimeUsed);
   }
}


bool EnvironmentCache::isInCache(const uint8_t* hash)
{
   bool found = false;

   lock();
   for(std::set<CacheEntry*>::const_iterator iterator = Cache.begin();
       iterator != Cache.end(); iterator++) {
      if(memcmp(hash, &(*iterator)->Hash, sizeof((*iterator)->Hash)) == 0) {
         found = true;
         break;
      }
   }
   unlock();

   return(found);
}


bool EnvironmentCache::storeInCache(const uint8_t* hash, const char* fileName)
{
   bool success = true;

   lock();
   if(!isInCache(hash)) {
      // ====== Create CacheEntry ===========================================
      CacheEntry* entry = new CacheEntry;
      assert(entry != NULL);
      memcpy(&entry->Hash, hash, sizeof(entry->Hash));
      entry->LastTimeUsed = getMicroTime();
      entry->FileName     = CacheDirectory;
      entry->FileName += "/";
      for(size_t i = 0; i < SE_HASH_SIZE; i++) {
         char hex[8];
         snprintf((char*)&hex, sizeof(hex), "%02x", (unsigned int)hash[i]);
         entry->FileName += hex;
      }
      entry->FileName += ".data";

      // ====== Copy file ===================================================
      entry->Size = copyFile(fileName, entry->FileName.c_str(), NULL);
      if(entry->Size > 0) {
         Cache.insert(entry);
      }
      else {
         delete entry;
         success = false;
      }
   }
   unlock();

   return(success);
}


bool EnvironmentCache::copyFromCache(const uint8_t* hash, const char* fileName)
{
   bool success = false;

   lock();
   for(std::set<CacheEntry*>::iterator iterator = Cache.begin();
       iterator != Cache.end(); iterator++) {
      CacheEntry* entry = *iterator;
      if(memcmp(hash, &(*iterator)->Hash, sizeof((*iterator)->Hash)) == 0) {
         if(copyFile(entry->FileName.c_str(), fileName, hash) > 0) {
            success = true;
            printf("GOT-FROM-CACHE!");
            entry->LastTimeUsed = getMicroTime();
         }
         break;
      }
   }
   unlock();

   return(success);
}


int main(int argc, char** argv)
{
   EnvironmentCache ec;

   ec.initialize("/tmp/CACHE");

   uint8_t hash1[SE_HASH_SIZE];
   uint8_t hash2[SE_HASH_SIZE];
   uint8_t hash3[SE_HASH_SIZE];
   memset(&hash1, 0, sizeof(hash1));
   memset(&hash2, 0, sizeof(hash2));
   memset(&hash3, 0, sizeof(hash3));
   hash1[7]=0xab;
   hash2[7]=0x99;
   hash3[7]=0x11;

   ec.storeInCache((const uint8_t*)&hash1, "xy");
   ec.storeInCache((const uint8_t*)&hash2, "xy");
   printf("FIND-1: %d\n", ec.isInCache(hash1));
   printf("FIND-2: %d\n", ec.isInCache(hash2));
   printf("FIND-3: %d\n", ec.isInCache(hash3));

   ec.print();

   ec.copyFromCache((const uint8_t*)&hash1, "/tmp/env1");
   ec.copyFromCache((const uint8_t*)&hash2, "/tmp/env1");
   ec.copyFromCache((const uint8_t*)&hash3, "/tmp/env1");
}
