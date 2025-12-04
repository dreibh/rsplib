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
 * Copyright (C) 2002-2026 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#ifndef LOGLEVEL_H
#define LOGLEVEL_H

#include "tdtypes.h"
#include "timeutilities.h"
#include "threadsafety.h"
#include "debug.h"

#include <errno.h>
#include <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif


#define LOGLEVEL_ERROR     1
#define LOGLEVEL_WARNING   2
#define LOGLEVEL_NOTE      3
#define LOGLEVEL_ACTION    4
#define LOGLEVEL_VERBOSE1  5
#define LOGLEVEL_VERBOSE2  6
#define LOGLEVEL_VERBOSE3  7
#define LOGLEVEL_VERBOSE4  8
#define LOGLEVEL_VERBOSE5  9

#ifndef MAX_LOGLEVEL
#warning MAX_LOGLEVEL should be defined in config.h!
#define MAX_LOGLEVEL LOGLEVEL_VERBOSE5
#endif


extern unsigned int        gLogLevel;
extern bool                gLogColorMode;
extern FILE**              gStdLog;
extern struct ThreadSafety gLogMutex;


void setLogColor(const unsigned int color);


#define stdlog (*gStdLog)
#define logerror(text) fprintf(stdlog, "%s: %s\n", text, strerror(errno))


#define LOG_BEGIN(prefix,c1,c2)                \
   {                                           \
      loggingMutexLock();                      \
      setLogColor(c1);                         \
      printTimeStamp(stdlog);                  \
      setLogColor(c2);                         \
      fprintf(stdlog,"P%lu.%lx@%s %s:%u %s()\n", \
              (unsigned long)getpid(),           \
              (unsigned long)pthread_self(),     \
              getHostName(),                   \
              __FILE__,                        \
              __LINE__,                        \
              __FUNCTION__                     \
              );                               \
      setLogColor(c1);                         \
      printTimeStamp(stdlog);                  \
      setLogColor(c2);                         \
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


#define LOG_ERROR    if((LOGLEVEL_ERROR    <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_ERROR))     LOG_BEGIN("Error: ", 9, 1)
#define LOG_WARNING  if((LOGLEVEL_WARNING  <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_WARNING))   LOG_BEGIN("Warning: ", 13, 5)
#define LOG_ACTION   if((LOGLEVEL_ACTION   <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_ACTION))    LOG_BEGIN("", 12, 4)
#define LOG_NOTE     if((LOGLEVEL_NOTE     <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_NOTE))      LOG_BEGIN("", 10, 2)
#define LOG_VERBOSE  LOG_VERBOSE1
#define LOG_VERBOSE1 if((LOGLEVEL_VERBOSE1 <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_VERBOSE1))  LOG_BEGIN("", 10, 3)
#define LOG_VERBOSE2 if((LOGLEVEL_VERBOSE2 <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_VERBOSE2))  LOG_BEGIN("", 14, 6)
#define LOG_VERBOSE3 if((LOGLEVEL_VERBOSE3 <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_VERBOSE3))  LOG_BEGIN("", 3, 3)
#define LOG_VERBOSE4 if((LOGLEVEL_VERBOSE4 <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_VERBOSE4))  LOG_BEGIN("", 6, 6)
#define LOG_VERBOSE5 if((LOGLEVEL_VERBOSE5 <= MAX_LOGLEVEL) && (gLogLevel >= LOGLEVEL_VERBOSE5))  LOG_BEGIN("", 7, 7)


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

/**
  * Obtain host name.
  */
const char* getHostName();


#ifdef __cplusplus
}
#endif


#endif
