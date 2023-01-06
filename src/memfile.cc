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
 * Copyright (C) 2002-2023 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#include "memfile.h"

#include <sys/stat.h>
#include <sys/mman.h>


// ###### Open mmap'ed file #################################################
void closeMemFile(MemFile* memFile)
{
   if(memFile) {
      if(memFile->Address) {
         munmap(memFile->Address, memFile->Length);
      }
      if(memFile->FileHandle) {
         fclose(memFile->FileHandle);
      }
      delete memFile;
   }
}


// ###### Close mmap'ed file ################################################
MemFile* openMemFile(const char* name)
{
   MemFile* memFile = new MemFile;

   if(memFile != NULL) {
      memFile->Address    = NULL;
      memFile->FileHandle = fopen(name, "r");
      if(memFile->FileHandle != NULL) {
         struct stat fileStat;
         if(fstat(fileno(memFile->FileHandle), &fileStat) == 0) {
            memFile->Length  = fileStat.st_size;
            memFile->Address = (char*)mmap(NULL, memFile->Length, PROT_READ, MAP_PRIVATE,
                                           fileno(memFile->FileHandle), 0);
            if(memFile->Address != NULL) {
               return(memFile);
            }
         }
      }
   }

   closeMemFile(memFile);   
   return(NULL);
}
