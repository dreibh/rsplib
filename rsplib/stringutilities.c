/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#include <string.h>
#include <stdlib.h>


/* ###### Length-checking strcpy() ###################################### */
int safestrcpy(char* dest, const char* src, const size_t size)
{
   if(size > 0) {
      strncpy(dest,src,size);
      dest[size - 1] = 0x00;
      return(strlen(dest) < size);
   }
   return(false);
}


/* ###### Length-checking strcat() ###################################### */
int safestrcat(char* dest, const char* src, const size_t size)
{
   const int l1  = strlen(dest);
   const int l2  = strlen(src);

   if(l1 + l2 < (int)size) {
      strcat(dest,src);
      return(true);
   }
   else if((int)size > l2) {
      strcat((char*)&dest[size - l2],src);
   }
   else {
      safestrcpy(dest,src,size);
   }
   return(false);
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
            return(false);
            break;
         }
      }
      if(i < bufferSize) {
         buffer[i++] = 0x00;
      }
      else {
         return(false);
      }
      *position = (size_t)((long)end - (long)input);
      while(input[*position] == ' ') {
         (*position)++;
      }
      return(true);
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
   return(false);
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
