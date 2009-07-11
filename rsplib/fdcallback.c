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
 * Copyright (C) 2002-2009 by Thomas Dreibholz
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
#include "fdcallback.h"


/* ###### Constructor #################################################### */
void fdCallbackNew(struct FDCallback* fdCallback,
                   struct Dispatcher* dispatcher,
                   const int          fd,
                   const unsigned int eventMask,
                   void               (*callback)(struct Dispatcher* dispatcher,
                                                  int                fd,
                                                  unsigned int       eventMask,
                                                  void*              userData),
                   void*              userData)
{
   struct SimpleRedBlackTreeNode* result;
   CHECK((fd >= 0) && (fd < (int)FD_SETSIZE));

   simpleRedBlackTreeNodeNew(&fdCallback->Node);
   fdCallback->Master          = dispatcher;
   fdCallback->FD              = fd;
   fdCallback->EventMask       = eventMask;
   fdCallback->Callback        = callback;
   fdCallback->UserData        = userData;
   fdCallback->SelectTimeStamp = getMicroTime();

   dispatcherLock(fdCallback->Master);
   result = simpleRedBlackTreeInsert(&fdCallback->Master->FDCallbackStorage,
                                     &fdCallback->Node);
   CHECK(result == &fdCallback->Node);
   fdCallback->Master->AddRemove = true;
   dispatcherUnlock(fdCallback->Master);
}


/* ###### Destructor ##################################################### */
void fdCallbackDelete(struct FDCallback* fdCallback)
{
   struct SimpleRedBlackTreeNode* result;
   CHECK(simpleRedBlackTreeNodeIsLinked(&fdCallback->Node));

   dispatcherLock(fdCallback->Master);
   result = simpleRedBlackTreeRemove(&fdCallback->Master->FDCallbackStorage,
                                         &fdCallback->Node);
   CHECK(result == &fdCallback->Node);
   fdCallback->Master->AddRemove = true;
   dispatcherUnlock(fdCallback->Master);

   simpleRedBlackTreeNodeDelete(&fdCallback->Node);
   fdCallback->Master          = NULL;
   fdCallback->FD              = -1;
   fdCallback->EventMask       = 0;
   fdCallback->Callback        = NULL;
   fdCallback->UserData        = NULL;
   fdCallback->SelectTimeStamp = 0;
}


/* ###### Update event mask ############################################## */
void fdCallbackUpdate(struct FDCallback* fdCallback,
                      const unsigned int eventMask)
{
   dispatcherLock(fdCallback->Master);
   fdCallback->EventMask = eventMask;
   dispatcherUnlock(fdCallback->Master);
}


/* ###### FDCallback comparision function ################################ */
int fdCallbackComparison(const void* fdCallbackPtr1, const void* fdCallbackPtr2)
{
   const struct FDCallback* fdCallback1 = (const struct FDCallback*)fdCallbackPtr1;
   const struct FDCallback* fdCallback2 = (const struct FDCallback*)fdCallbackPtr2;
   if(fdCallback1->FD < fdCallback2->FD) {
      return(-1);
   }
   else if(fdCallback1->FD > fdCallback2->FD) {
      return(1);
   }
   return(0);
}
