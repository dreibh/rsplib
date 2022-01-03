/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2022 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#include <map>

#include "environmentcache.h"
#include "scriptingservice.h"
#include "randomizer.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "stringutilities.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>


// ###### Constructor #######################################################
EnvironmentCache::EnvironmentCache()
{
   CacheDirectory      = "";
   ClearCacheDirectory = false;
   TotalSize           = 0;
}


// ###### Destructor ########################################################
EnvironmentCache::~EnvironmentCache()
{
   cleanUp();
}


// ###### Initialize cache directory ########################################
bool EnvironmentCache::initializeCache(const char*              directory,
                                       const unsigned long long maxSize,
                                       const unsigned int       maxEntries,
                                       FILE*                    logFile)
{
   CacheDirectory = directory;
   LogFile        = logFile;
   MaxSize        = maxSize;
   MaxEntries     = maxEntries;

   // ====== Create temporary cache =========================================
   if(CacheDirectory == "") {
      char        tempName[256];
      const char* tempDirectory = getenv("TMPDIR");
      if(tempDirectory == NULL) {
         safestrcpy((char*)&tempName, "/tmp", sizeof(tempName));
      }
      else {
         safestrcpy((char*)&tempName, tempDirectory, sizeof(tempName));
      }
      safestrcat((char*)&tempName, "/rspSS-EnvironmentCache-XXXXXX", sizeof(tempName));
      if(mkdtemp((char*)&tempName) == NULL) {
         return(false);
      }
      CacheDirectory      = tempName;
      ClearCacheDirectory = true;

      if(logFile) {
         printTimeStamp(logFile);
         fprintf(logFile, "Initialized temporary cache in %s\n", tempName);
      }
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
               // Not the optimal solution, but should work ...
               entry->LastTimeUsed = random64() % getMicroTime();
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

      if(logFile) {
         printTimeStamp(logFile);
         fprintf(logFile, "Initialized permanent cache in %s\n",
                 CacheDirectory.c_str());
         print(logFile);
      }
   }

   const unsigned long long bytesToBeFreed   = (TotalSize > MaxSize) ? TotalSize - MaxSize : 0;
   const unsigned int       entriesToBeFreed = (Cache.size() > MaxEntries) ? Cache.size() - MaxEntries : 0;
   purge(bytesToBeFreed, entriesToBeFreed);
   return(true);
}


// ###### Clean up ##########################################################
void EnvironmentCache::cleanUp()
{
   // ====== Clear cache ====================================================
   std::set<CacheEntry*>::iterator iterator = Cache.begin();
   while(iterator != Cache.end()) {
      delete *iterator;
      Cache.erase(iterator);
      iterator = Cache.begin();
   }
   TotalSize = 0;

   // ====== Remove cache directory =========================================
   if(ClearCacheDirectory) {
      DIR* directory = opendir(CacheDirectory.c_str());
      if(directory != NULL) {
         char* cwd = getcwd(NULL, 0);
         if(chdir(CacheDirectory.c_str()) == 0) {
            dirent* dentry = readdir(directory);
            while(dentry != NULL) {
               unlink(dentry->d_name);
               dentry = readdir(directory);
            }
            closedir(directory);

            if(chdir(cwd) != 0) {
               // Something went wrong ...
            }
         }
         if(cwd) {
            free(cwd);
         }
      }
   }
}


// ###### Purge out-of-date entries #########################################
bool EnvironmentCache::purge(unsigned long long bytesToBeFreed,
                             unsigned int       entriesToBeFreed)
{
   std::map<unsigned long long, CacheEntry*> lru;
   for(std::set<CacheEntry*>::iterator iterator = Cache.begin();
       iterator != Cache.end(); iterator++) {
      CacheEntry* entry = *iterator;
      lru.insert(std::pair<unsigned long long, CacheEntry*>(entry->LastTimeUsed, entry));
   }

   for(std::map<unsigned long long, CacheEntry*>::iterator iterator = lru.begin();
       iterator != lru.end(); iterator++) {
      CacheEntry* entry = iterator->second;

      if( (bytesToBeFreed == 0) && (entriesToBeFreed == 0) ) {
         return(true);
      }

      unlink(entry->FileName.c_str());
      TotalSize -= entry->Size;
      bytesToBeFreed   -= std::min(bytesToBeFreed, entry->Size);
      entriesToBeFreed -= std::min(entriesToBeFreed, (unsigned int)1);
      if(LogFile) {
         printTimeStamp(LogFile);
         fputs("Purging environment ", LogFile);
         printHash(LogFile, (const uint8_t*)&entry->Hash);
         fprintf(LogFile, " (%llu KiB) => %llu KiB in %u entries\n",
                 entry->Size / 1024, TotalSize / 1024, (unsigned int)Cache.size() - 1);
      }

      Cache.erase(entry);
      delete entry;
   }

   if( (bytesToBeFreed == 0) && (entriesToBeFreed == 0) ) {
      return(true);
   }
   return(false);
}


void EnvironmentCache::printHash(FILE* fh, const uint8_t* hash)
{
   for(size_t i = 0; i < SE_HASH_SIZE; i++) {
      fprintf(fh, "%02x", (unsigned int)hash[i]);
   }
}


// ###### Copy file and calculate hash ######################################
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


// ###### Print cache contents ##############################################
void EnvironmentCache::print(FILE* fh)
{
   fprintf(fh, "Cache contents [%llu KiB in %u entries]:\n",
           TotalSize / 1024, (unsigned int)Cache.size());

   unsigned int i = 1;
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


// ###### Look for entry in cache ###########################################
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


// ###### Store environment into cache ######################################
bool EnvironmentCache::storeInCache(const uint8_t*           hash,
                                    const char*              fileName,
                                    const unsigned long long size)
{
   bool success = true;

   lock();
   if(!isInCache(hash)) {
      // ====== Purge old contents ==========================================
      if( ((Cache.size() + 1) > MaxEntries) ||
          (TotalSize + size > MaxSize) ) {
         purge((TotalSize + size > MaxSize)    ? TotalSize + size - MaxSize : 0,
               (Cache.size() + 1 > MaxEntries) ? Cache.size() + 1 - MaxEntries : 0);
      }

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
         if(entry->Size != size) {
            if(LogFile) {
               printTimeStamp(LogFile);
               fprintf(LogFile, "CACHE WARNING: Expected size %llu != actual size %llu!\n",
                       size, entry->Size);
            }
         }
         Cache.insert(entry);
         TotalSize += entry->Size;

         if(LogFile) {
            printTimeStamp(LogFile);
            fputs("Caching environment ", LogFile);
            printHash(LogFile, (const uint8_t*)&entry->Hash);
            fprintf(LogFile, " (%llu KiB) => %llu KiB in %u entries\n",
                    entry->Size / 1024, TotalSize / 1024, (unsigned int)Cache.size());
         }
      }
      else {
         delete entry;
         success = false;
      }
   }
   unlock();

   return(success);
}


// ###### Get environment from cache ########################################
bool EnvironmentCache::copyFromCache(const uint8_t* hash,
                                     const char*    fileName)
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
               if(LogFile) {
                  printTimeStamp(LogFile);
                  fputs("Retrieving environment ", LogFile);
                  printHash(LogFile, (const uint8_t*)&entry->Hash);
                  fprintf(LogFile, " (%llu KiB) => %llu KiB in %u entries\n",
                          entry->Size / 1024, TotalSize / 1024, (unsigned int)Cache.size());
               }
               success = true;
               entry->LastTimeUsed = getMicroTime();
            }
            else {
               if(LogFile) {
                  printTimeStamp(LogFile);
                  fputs("CACHE ERROR: Expected hash ", LogFile);
                  printHash(LogFile, (uint8_t*)hash);
                  fputs(", but computed hash is ", LogFile);
                  printHash(LogFile, (uint8_t*)&newHash);
                  fputs("!\n", LogFile);
               }
            }
         }
         if(!success) {
            TotalSize -= entry->Size;
            if(LogFile) {
               printTimeStamp(LogFile);
               fputs("Removing damaged environment ", LogFile);
               printHash(LogFile, (const uint8_t*)&entry->Hash);
               fprintf(LogFile, " (%llu KiB) => %llu KiB in %u entries\n",
                       entry->Size / 1024, TotalSize / 1024, (unsigned int)Cache.size() - 1);
            }
            Cache.erase(iterator);
            unlink(entry->FileName.c_str());
            delete entry;
         }
         break;
      }
   }
   unlock();

   return(success);
}
