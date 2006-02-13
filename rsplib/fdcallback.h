/*
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
 * Purpose: FDCallback
 *
 */


#ifndef FDCALLBACK_H
#define FDCALLBACK_H


#include "tdtypes.h"
#include "dispatcher.h"
#include "simpleredblacktree.h"

#include <sys/poll.h>


#ifdef __cplusplus
extern "C" {
#endif


enum FDCallbackEvents
{
   FDCE_Read      = POLLIN,
   FDCE_Write     = POLLOUT,
   FDCE_Exception = POLLPRI|POLLHUP
};


struct FDCallback
{
   struct SimpleRedBlackTreeNode Node;

   struct Dispatcher*            Master;
   int                           FD;
   unsigned int                  EventMask;
   void                          (*Callback)(struct Dispatcher* dispatcher,
                                             int                fd,
                                             unsigned int       eventMask,
                                             void*              userData);
   unsigned long long            SelectTimeStamp;
   void*                         UserData;
};


/**
  * Constructor.
  *
  * @param fdCallback FDCallback.
  * @param dispatcher Dispatcher.
  * @param fd File descriptor.
  * @param eventMask Event mask.
  * @param callback Callback.
  * @param userData User data for callback.
  */
void fdCallbackNew(struct FDCallback* fdCallback,
                   struct Dispatcher* dispatcher,
                   const int          fd,
                   const unsigned int eventMask,
                   void               (*callback)(struct Dispatcher* dispatcher,
                                                  int                fd,
                                                  unsigned int       eventMask,
                                                  void*              userData),
                   void*              userData);

/**
  * Destructor.
  *
  * @param fdCallback FDCallback.
  */
void fdCallbackDelete(struct FDCallback* fdCallback);

/**
  * Update event mask.
  *
  * @param fdCallback FDCallback.
  * @param
  */
void fdCallbackUpdate(struct FDCallback* fdCallback,
                      const unsigned int eventMask);

/**
  * FDCallback comparision function.
  *
  * @param fdCallbackPtr1 Pointer to FDCallback 1.
  * @param fdCallbackPtr2 Pointer to FDCallback 2.
  * @return Comparision result.
  */
int fdCallbackComparison(const void* fdCallbackPtr1, const void* fdCallbackPtr2);


#ifdef __cplusplus
}
#endif


#endif
