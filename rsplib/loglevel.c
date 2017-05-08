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
 * Copyright (C) 2002-2017 by Thomas Dreibholz
 *
 * Acknowledgements:
 * Realized in co-operation between Siemens AG and
 * University of Essen, Institute of Computer Networking Technology.
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany
 * (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
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

#include "tdtypes.h"
#include "loglevel.h"
#include "threadsafety.h"
#include "stringutilities.h"

#include <unistd.h>
#include <sys/utsname.h>


#ifdef HAVE_STDERR_FILEPTR
/* stderr is of type FILE* */
FILE** gStdLog = &stderr;
#else
/* stderr is a macro returning standard error FILE* */
static FILE* _stderr  = stderr;
FILE**        gStdLog = &_stderr;
#endif


unsigned int          gLogLevel      = LOGLEVEL_ERROR;
static bool           gColorMode     = true;
static bool           gCloseStdLog   = false;
struct ThreadSafety   gLogMutex;
static char           gHostName[128] = { 0x00 };


/* ###### Set ASCII color ################################################ */
void setLogColor(const unsigned int color)
{
   if(gColorMode) {
      fprintf(stdlog,"\x1b[%dm",
              30 + (color % 8) + ((color > 8) ? 60 : 0));
   }
}


/* ###### Initialize logfile ############################################# */
static bool initLogFile(const unsigned int logLevel, const char* fileName, const char* fileMode)
{
   FILE* newLogFile;

   finishLogging();
   if(fileName != NULL) {
      newLogFile = fopen(fileName,fileMode);
      if(newLogFile != NULL) {
         *gStdLog     = newLogFile;
         gCloseStdLog = true;
         gLogLevel    = min(logLevel,MAX_LOGLEVEL);
         return(true);
      }
   }
   return(false);
}


/* ###### Set logging parameter ########################################## */
bool initLogging(const char* parameter)
{
   if(!(strncmp(parameter,"-logfile=",9))) {
      return(initLogFile(gLogLevel,(char*)&parameter[9],"w"));
   }
   else if(!(strncmp(parameter,"-logappend=",11))) {
      return(initLogFile(gLogLevel,(char*)&parameter[11],"a"));
   }
   else if(!(strcmp(parameter,"-logquiet"))) {
      initLogFile(0,NULL,"w");
      gLogLevel = 0;
   }
   else if(!(strncmp(parameter,"-loglevel=",10))) {
      gLogLevel = min(atol((char*)&parameter[10]),MAX_LOGLEVEL);
   }
   else if(!(strncmp(parameter,"-logcolor=",10))) {
      if(!(strcmp((char*)&parameter[10],"off"))) {
         gColorMode = false;
      }
      else {
         gColorMode = true;
      }
   }
   else {
      fprintf(stderr, "ERROR: Invalid logging parameter %s\n", parameter);
      return(false);
   }
   return(true);
}


/* ###### Begin logging ################################################## */
void beginLogging()
{
   struct utsname hostInfo;

   threadSafetyNew(&gLogMutex, "_LogPrinter_");
   if((gCloseStdLog) && (ftell(*gStdLog) > 0)) {
      fputs("\n#########################################################################################\n\n",*gStdLog);
   }
   if(uname(&hostInfo) != 0) {
      strcpy((char*)&gHostName, "?");
   }
   else {
      snprintf((char*)&gHostName, sizeof(gHostName), "%s",
               hostInfo.nodename);
   }
   LOG_NOTE
   fprintf(stdlog,"Logging started, log level is %d.\n",gLogLevel);
   LOG_END
}


/* ###### Finish logging ################################################# */
void finishLogging()
{
   if((*gStdLog) && (gCloseStdLog)) {
      LOG_ACTION
      fputs("Logging finished.\n",stdlog);
      LOG_END
      fclose(*gStdLog);
      gCloseStdLog = false;
      *gStdLog     = stderr;
   }
   threadSafetyDelete(&gLogMutex);
}


/* ###### Obtain mutex for debug output ################################## */
void loggingMutexLock()
{
   threadSafetyLock(&gLogMutex);
}


/* ###### Release mutex for debug output ################################# */
void loggingMutexUnlock()
{
   threadSafetyUnlock(&gLogMutex);
}


/* ###### Get host info string ########################################### */
const char* getHostName()
{
   return((const char*)gHostName);
}
