/*
 *  $Id: utilities.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
 *
 * RSerPool implementation.
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at http://www.sctp.de/rserpool.html
 * which should be used for any discussion related to this implementation.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Utilities
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "utilities.h"

#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <sched.h>
#include <time.h>

#include <ext_socket.h>
#include <sys/uio.h>
#include <glib.h>



/* ###### Get current timer ############################################## */
card64 getMicroTime()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return(((card64)tv.tv_sec * (card64)1000000) + (card64)tv.tv_usec);
}


/* ###### Print time stamp ############################################### */
void printTimeStamp(FILE* fd)
{
   char str[64];
   const card64 microTime = getMicroTime();
   const time_t timeStamp = microTime / 1000000;
   const struct tm *timeptr = localtime(&timeStamp);

   strftime((char*)&str,sizeof(str),"%d-%b-%Y %H:%M:%S",timeptr);
   fprintf(fd,str);
   fprintf(fd,".%04d: ",(unsigned int)(microTime % 1000000) / 100);
}


/* ###### Length-checking strcpy() ###################################### */
bool safestrcpy(char* dest, const char* src, const size_t size)
{
   if(size > 0) {
      strncpy(dest,src,size);
      dest[size - 1] = 0x00;
      return(strlen(dest) < size);
   }
   return(false);
}


/* ###### Length-checking strcat() ###################################### */
bool safestrcat(char* dest, const char* src, const size_t size)
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
bool getNextWord(const char* input, char* buffer, const size_t bufferSize, size_t* position)
{
   size_t      i;
   char*       c;
   bool        result;
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


/* ###### File descriptor pointer comparision function ################### */
gint fdCompareFunc(gconstpointer a,
                   gconstpointer b)
{
   const int f1 = (int)a;
   const int f2 = (int)b;

   if(f1 < f2) {
      return(-1);
   }
   else if(f1 > f2) {
      return(1);
   }
   return(0);
}


/* ###### GTree traverse function to get the first key/value pair ######## */
static gint getFirstTreeElementTraverseFunc(gpointer key, gpointer value, gpointer data)
{
   ((gpointer*)data)[0] = key;
   ((gpointer*)data)[1] = value;
   return(true);
}


/* ###### Remove first key/value pair from tree and return value ######### */
bool getFirstTreeElement(GTree* tree, gpointer* key, gpointer* value)
{
   gpointer data[2] = { NULL, NULL };

   g_tree_traverse(tree,getFirstTreeElementTraverseFunc,G_IN_ORDER,&data);
   if(data[0] != NULL) {
      *key   = data[0];
      *value = data[1];
      return(true);
   }
   *key   = NULL;
   *value = NULL;
   return(false);
}
