/*
 *  $Id: loglevel.c,v 1.5 2004/11/10 14:44:38 dreibh Exp $
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
 * Purpose: Logging Management
 *
 */


#include "tdtypes.h"
#include "loglevel.h"
#include "threadsafety.h"
#include "stringutilities.h"


#ifdef HAVE_STDERR_FILEPTR
/* stderr is of type FILE* */
FILE** gStdLog = &stderr;
#else
/* stderr is a macro returning standard error FILE* */
static FILE* _stderr = stderr;
FILE**       gStdLog = &_stderr;
#endif


unsigned int        gLogLevel    = LOGLEVEL_NOTE;
static bool         gColorMode   = true;
static bool         gCloseStdLog = false;
struct ThreadSafety gLogMutex;


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
   finishLogging();
   if(fileName != NULL) {
      *gStdLog = fopen(fileName,fileMode);
      if(*gStdLog != NULL) {
         gCloseStdLog = true;
         gLogLevel   = min(logLevel,MAX_LOGLEVEL);
         return(true);
      }
   }

   *gStdLog     = stderr;
   gCloseStdLog = false;
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
      printf("ERROR: Invalid logging parameter: %s\n",parameter);
      return(false);
   }
   return(true);
}


/* ###### Begin logging ################################################## */
void beginLogging()
{
   threadSafetyInit(&gLogMutex, "_LogPrinter_");
   if((gCloseStdLog) && (ftell(*gStdLog) > 0)) {
      fputs("\n#########################################################################################\n\n",*gStdLog);
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
   threadSafetyDestroy(&gLogMutex);
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
