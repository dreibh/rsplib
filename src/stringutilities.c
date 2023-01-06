/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2023 by Thomas Dreibholz
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

#include "stringutilities.h"
#include "debug.h"

#include <string.h>
#include <stdlib.h>


/* ###### Length-checking strcpy() ###################################### */
int safestrcpy(char* dest, const char* src, const size_t size)
{
   CHECK(size > 0);
   strncpy(dest, src, size);
   dest[size - 1] = 0x00;
   return(strlen(dest) < size);
}


/* ###### Length-checking strcat() ###################################### */
int safestrcat(char* dest, const char* src, const size_t size)
{
   const size_t l1 = strlen(dest);
   const size_t l2 = strlen(src);

   CHECK(size > 0);
   strncat(dest, src, size - l1 - 1);
   dest[size - 1] = 0x00;
   return(l1 + l2 < size);
}


/* ###### Find first occurrence of character in string ################### */
char* strindex(char* string, const char character)
{
   if(string != NULL) {
      while(*string != character) {
         if(*string == 0x00) {
            return(NULL);
         }
         string++;
      }
      return(string);
   }
   return(NULL);
}


/* ###### Find last occurrence of character in string #################### */
char* strrindex(char* string, const char character)
{
   const char* original = string;

   if(original != NULL) {
      string = (char*)&string[strlen(string)];
      while(*string != character) {
         if(string == original) {
            return(NULL);
         }
         string--;
      }
      return(string);
   }
   return(NULL);
}


/* ###### Get next word of text string ################################### */
int getNextWord(const char* input, char* buffer, const size_t bufferSize, size_t* position)
{
   size_t      i;
   char*       c;
   int        result;
   const char* end;

   end = strindex((char*)&input[*position],' ');
   if(end != NULL) {
      i = 0;
      for(c = (char*)&input[*position];c < end;c++) {
         if(i < bufferSize) {
            buffer[i++] = *c;
         }
         else {
            return(0);
         }
      }
      if(i < bufferSize) {
         buffer[i++] = 0x00;
      }
      else {
         return(0);
      }
      *position = (size_t)((long)end - (long)input);
      while(input[*position] == ' ') {
         (*position)++;
      }
      return(1);
   }
   else {
      i = strlen((char*)&input[*position]);
      if(i > 0) {
         result = safestrcpy(buffer,(char*)&input[*position],bufferSize);
         *position += i;
         return(result);
      }
      safestrcpy(buffer,"",bufferSize);
   }
   return(0);
}


/* ###### Memory duplicator ############################################## */
void* memdup(const void* source, const size_t size)
{
   void* dest = malloc(size);
   if(dest != NULL) {
      memcpy(dest,source,size);
   }
   return(dest);
}
