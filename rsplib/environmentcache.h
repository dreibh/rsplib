/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2010 by Thomas Dreibholz
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

#ifndef ENVIRONMENTCACHE_H
#define ENVIRONMENTCACHE_H

#include "scriptingpackets.h"
#include "mutex.h"

#include <string.h>
#include <set>
#include <string>


class EnvironmentCache : public TDMutex
{
   public:
   EnvironmentCache();
   ~EnvironmentCache();

   bool initializeCache(const char*              directory,
                        const unsigned long long maxSize,
                        const unsigned int       maxEntries,
                        FILE*                    logFile);
   void cleanUp();

   void print(FILE* fh);

   bool isInCache(const uint8_t* hash);

   bool storeInCache(const uint8_t*           hash,
                     const char*              fileName,
                     const unsigned long long size);
   bool copyFromCache(const uint8_t* hash,
                      const char*    fileName);

   private:
   static void printHash(FILE* fh, const uint8_t* hash);
   static unsigned long long copyFile(const char* source,
                                      const char* destination,
                                      uint8_t*    hash);
   bool purge(unsigned long long bytesToBeFreed,
              unsigned int       entriesToBeFreed);


   struct CacheEntry {
      inline bool operator()(const CacheEntry* e1,
                             const CacheEntry* e2) {
         return(memcmp(&e1->Hash, &e2->Hash, sizeof(e1->Hash)) < 0);
      }

      uint8_t            Hash[SE_HASH_SIZE];
      unsigned long long Size;
      unsigned long long LastTimeUsed;
      std::string        FileName;
   };

   std::set<CacheEntry*, CacheEntry> Cache;
   std::string           CacheDirectory;
   bool                  ClearCacheDirectory;
   unsigned long long    TotalSize;
   FILE*                 LogFile;
   unsigned long long    MaxSize;
   unsigned int          MaxEntries;
};

#endif
