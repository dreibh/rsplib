/*
 *  $Id: loglevel.h,v 1.2 2004/07/18 15:30:43 dreibh Exp $
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


#ifndef LOGLEVEL_H
#define LOGLEVEL_H

#include "tdtypes.h"
#include "timeutilities.h"
#include "threadsafety.h"


#ifdef __cplusplus
extern "C" {
#endif


#define LOGLEVEL_ERROR     1
#define LOGLEVEL_WARNING   2
#define LOGLEVEL_ACTION    3
#define LOGLEVEL_NOTE      4
#define LOGLEVEL_VERBOSE1  5
#define LOGLEVEL_VERBOSE2  6
#define LOGLEVEL_VERBOSE3  7
#define LOGLEVEL_VERBOSE4  8
#define LOGLEVEL_VERBOSE5  9
#define LOGLEVEL_MUTEX    10

#define MAX_LOGLEVEL 10


extern unsigned int        gLogLevel;
extern bool                gLogColorMode;
extern FILE**              gStdLog;
extern struct ThreadSafety gLogMutex;


void setLogColor(const unsigned int color);


#define stdlog (*gStdLog)
#define logerror(text) fprintf(stdlog,"%s: %s\n",text,strerror(errno))


#define LOG_BEGIN(prefix,c1,c2)              \
   {                                         \
      loggingMutexLock();                    \
      setLogColor(c1);                       \
      printTimeStamp(stdlog);                \
      fprintf(stdlog,"P%u.%u %s:%u %s() - ", \
              (unsigned int)getpid(),        \
              (unsigned int)pthread_self(),  \
              __FILE__,                      \
              __LINE__,                      \
              __FUNCTION__                   \
              );                             \
      setLogColor(c2);                       \
      fputs(prefix,stdlog);

#define LOG_END             \
      setLogColor(0);       \
      fflush(stdlog);       \
      loggingMutexUnlock(); \
   }

#define LOG_END_FATAL                             \
      fputs("FATAL ERROR - ABORTING!\n", stdlog); \
      setLogColor(0);                             \
      fflush(stdlog);                             \
      abort();                                    \
   }


#if (LOG_ERROR <= MAX_LOGLEVEL)
#define LOG_ERROR    if(gLogLevel >= LOGLEVEL_ERROR)        LOG_BEGIN("Error: ",9,1)
#else
#define LOG_ERROR    if(0) {
#endif

#if (LOG_ACTION <= MAX_LOGLEVEL)
#define LOG_ACTION   if(gLogLevel >= LOGLEVEL_ACTION)       LOG_BEGIN("",12,4)
#else
#define LOG_ACTION   if(0) {
#endif

#if (LOG_WARNING <= MAX_LOGLEVEL)
#define LOG_WARNING  if(gLogLevel >= LOGLEVEL_WARNING)      LOG_BEGIN("Warning: ",13,5)
#else
#define LOG_WARNING  if(0) {
#endif

#if (LOG_NOTE <= MAX_LOGLEVEL)
#define LOG_NOTE     if(gLogLevel >= LOGLEVEL_NOTE)         LOG_BEGIN("",10,2)
#else
#define LOG_NOTE     if(0) {
#endif

#if (LOG_VERBOSE1 <= MAX_LOGLEVEL)
#define LOG_VERBOSE1 if(gLogLevel >= LOGLEVEL_VERBOSE1)     LOG_BEGIN("",10,3)
#else
#define LOG_VERBOSE1 if(0) {
#endif
#define LOG_VERBOSE LOG_VERBOSE1

#if (LOG_VERBOSE2 <= MAX_LOGLEVEL)
#define LOG_VERBOSE2 if(gLogLevel >= LOGLEVEL_VERBOSE2)     LOG_BEGIN("",14,6)
#else
#define LOG_VERBOSE2 if(0) {
#endif

#if (LOG_VERBOSE3 <= MAX_LOGLEVEL)
#define LOG_VERBOSE3 if(gLogLevel >= LOGLEVEL_VERBOSE3)     LOG_BEGIN("",11,3)
#else
#define LOG_VERBOSE3 if(0) {
#endif

#if (LOG_VERBOSE4 <= MAX_LOGLEVEL)
#define LOG_VERBOSE4 if(gLogLevel >= LOGLEVEL_VERBOSE4)     LOG_BEGIN("",6,11)
#else
#define LOG_VERBOSE4 if(0) {
#endif

#if (LOG_VERBOSE5 <= MAX_LOGLEVEL)
#define LOG_VERBOSE5 if(gLogLevel >= LOGLEVEL_VERBOSE5)     LOG_BEGIN("",7,7)
#else
#define LOG_VERBOSE5 if(0) {
#endif

#if (LOG_MUTEX <= MAX_LOGLEVEL)
#define LOG_MUTEX if(gLogLevel >= LOGLEVEL_MUTEX)           LOG_BEGIN("",7,7)
#else
#define LOG_MUTEX if(0) {
#endif



/**
  * Set logging parameter.
  *
  * @parameter Parameter.
  * @return true in case of success; false otherwise.
  */
bool initLogging(const char* parameter);

/**
  * Begin logging.
  */
void beginLogging();

/**
  * Finish logging.
  */
void finishLogging();

/**
  * Obtain mutex for debug output.
  */
void loggingMutexLock();

/**
  * Release mutex for debug output.
  */
void loggingMutexUnlock();


#ifdef __cplusplus
}
#endif


#endif
