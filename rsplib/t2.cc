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
#include <dirent.h>

#include <set>
#include <string>


class EnvironmentCache : public TDMutex
{
   public:
   EnvironmentCache();
   ~EnvironmentCache();

   bool initializeCache(const char* directory);

   void print(FILE* fh);

   bool isInCache(const uint8_t* hash);

   bool storeInCache(const uint8_t* hash, const char* fileName);
   bool copyFromCache(const uint8_t* hash, const char* fileName);

   private:
   static void printHash(FILE* fh, const uint8_t* hash);
   static unsigned long long copyFile(const char* source,
                                      const char* destination,
                                      uint8_t*    hash);

   struct CacheEntry {
      uint8_t            Hash[SE_HASH_SIZE];
      unsigned long long Size;
      unsigned long long LastTimeUsed;
      std::string        FileName;
   };
   std::set<CacheEntry*> Cache;
   std::string           CacheDirectory;
   bool                  ClearCacheDirectory;
   unsigned long long    TotalSize;
};


EnvironmentCache::EnvironmentCache()
{
   CacheDirectory      = "";
   ClearCacheDirectory = false;
   TotalSize           = 0;
}


EnvironmentCache::~EnvironmentCache()
{
   // ====== Clear cache ====================================================
   std::set<CacheEntry*>::iterator iterator = Cache.begin();
   while(iterator != Cache.end()) {
      delete *iterator;
      Cache.erase(iterator);
      iterator = Cache.begin();
   }

   // ====== Remove cache directory =========================================
   if(ClearCacheDirectory) {
      DIR* directory = opendir(CacheDirectory.c_str());
      if(directory != NULL) {
         char* cwd = getcwd(NULL, 0);
         chdir(CacheDirectory.c_str());

         dirent* dentry = readdir(directory);
         while(dentry != NULL) {
            unlink(dentry->d_name);
            dentry = readdir(directory);
         }
         closedir(directory);

         if(cwd) {
            chdir(cwd);
            free(cwd);
         }
      }
   }
}


bool EnvironmentCache::initializeCache(const char* directory)
{
   CacheDirectory = directory;

   // ====== Create temporary cache =========================================
   if(CacheDirectory == "") {
      char tempName[128];
      safestrcpy((char*)&tempName, "/tmp/rspSS-XXXXXX", sizeof(tempName));
      if(mkdtemp((char*)&tempName) == NULL) {
         return(false);
      }
      CacheDirectory      = tempName;
      ClearCacheDirectory = true;
   }

   // ====== Initialize cache from permanent cache directory ================
   else {
      if( (mkdir(CacheDirectory.c_str(), S_IRWXU) != 0) &&
          (errno != EEXIST) ) {
         return(false);
      }
      DIR* directory = opendir(CacheDirectory.c_str());
      if(directory != NULL) {
         dirent* dentry = readdir(directory);
         while(dentry != NULL) {
            const size_t length = strlen(dentry->d_name);
            if( (length == (2 * SE_HASH_SIZE) + 5) &&
                (dentry->d_name[(2 * SE_HASH_SIZE) + 0] == '.') &&
                (dentry->d_name[(2 * SE_HASH_SIZE) + 1] == 'd') &&
                (dentry->d_name[(2 * SE_HASH_SIZE) + 2] == 'a') &&
                (dentry->d_name[(2 * SE_HASH_SIZE) + 3] == 't') &&
                (dentry->d_name[(2 * SE_HASH_SIZE) + 4] == 'a') ) {
               CacheEntry* entry = new CacheEntry;
               assert(entry != NULL);
               entry->LastTimeUsed = getMicroTime();
               entry->FileName     = CacheDirectory + "/" + dentry->d_name;
               entry->Size         = sha1_computeHashOfFile(entry->FileName.c_str(),
                                                            (uint8_t*)&entry->Hash);
               if(entry->Size > 0) {
                  Cache.insert(entry);
                  TotalSize += entry->Size;
               }
            }

            dentry = readdir(directory);
         }
         closedir(directory);
      }
   }

   return(true);
}


void EnvironmentCache::printHash(FILE* fh, const uint8_t* hash)
{
   for(size_t i = 0; i < SE_HASH_SIZE; i++) {
      fprintf(fh, "%02x", (unsigned int)hash[i]);
   }
}


unsigned long long EnvironmentCache::copyFile(const char* source,
                                              const char* destination,
                                              uint8_t*    hash)
{
   unsigned long long size = 0;
   sha1_ctx           context;

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
            sha1_final(&context, hash);
         }
         if(fclose(output) != 0) {
            size = 0;
         }
      }
      fclose(input);
   }
   return(size);
}


void EnvironmentCache::print(FILE* fh)
{
   fprintf(fh, "Cache contents [%lluB in %u entries]\n",
           TotalSize, (unsigned int)Cache.size());

   unsigned int i = 0;
   for(std::set<CacheEntry*>::const_iterator iterator = Cache.begin();
       iterator != Cache.end(); iterator++) {
      const CacheEntry* entry = *iterator;
      fprintf(fh, "#%02u: ", i);
      printHash(fh, (uint8_t*)&entry->Hash);
      fprintf(fh, "\t%s\t%llu\t%llu\n",
              entry->FileName.c_str(),
              entry->Size, entry->LastTimeUsed);
      i++;
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
         TotalSize += entry->Size;
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
         uint8_t* newHash[SE_HASH_SIZE];
         if(copyFile(entry->FileName.c_str(), fileName, (uint8_t*)&newHash) > 0) {
            if(memcmp(hash, &newHash, SE_HASH_SIZE) == 0) {
               success = true;
               entry->LastTimeUsed = getMicroTime();
            }
            else {
               printTimeStamp(stderr);
               fputs("CACHE ERROR: Expected hash ", stderr);
               printHash(stderr, (uint8_t*)hash);
               fputs(", but computed hash is ", stderr);
               printHash(stderr, (uint8_t*)&newHash);
               fputs("!\n", stderr);

               Cache.erase(iterator);
               unlink(entry->FileName.c_str());
               delete entry;
            }
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

   ec.initializeCache("/tmp/CACHE");
   ec.print(stdout);


   puts("--- START ---");

   uint8_t hash1[SE_HASH_SIZE];
   uint8_t hash2[SE_HASH_SIZE];
   uint8_t hash3[SE_HASH_SIZE];
   uint8_t hash4[SE_HASH_SIZE];

   if(sha1_computeHashOfFile("xy1", (uint8_t*)&hash1) == 0) {
      puts("File xy1 not found!");
      exit(1);
   }
   if(sha1_computeHashOfFile("xy2", (uint8_t*)&hash2) == 0) {
      puts("File xy2 not found!");
      exit(1);
   }
   if(sha1_computeHashOfFile("xy3", (uint8_t*)&hash3) == 0) {
      puts("File xy3 not found!");
      exit(1);
   }
   memset(&hash4, 0xab, sizeof(hash4));

   ec.storeInCache((const uint8_t*)&hash1, "xy1");
   ec.storeInCache((const uint8_t*)&hash2, "xy2");
   ec.storeInCache((const uint8_t*)&hash3, "xy3");
   printf("FIND-1: %d\n", ec.isInCache(hash1));
   printf("FIND-2: %d\n", ec.isInCache(hash2));
   printf("FIND-3: %d\n", ec.isInCache(hash3));
   printf("FIND-4: %d\n", ec.isInCache(hash4));

   ec.print(stdout);

   ec.copyFromCache((const uint8_t*)&hash1, "/tmp/env1");
   ec.copyFromCache((const uint8_t*)&hash2, "/tmp/env2");
   ec.copyFromCache((const uint8_t*)&hash3, "/tmp/env3");
   ec.copyFromCache((const uint8_t*)&hash4, "/tmp/env4");

   puts("--- ENDE ---");
   ec.print(stdout);
}
