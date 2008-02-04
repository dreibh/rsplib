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
 * Copyright (C) 2002-2008 by Thomas Dreibholz
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

#ifndef BREAKDETECTOR_H
#define BREAKDETECTOR_H


#include "tdtypes.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
  * Install break handler.
  */
void installBreakDetector();

/**
  * Uninstall break handler.
  */
void uninstallBreakDetector();

/**
  * Check, if break has been detected.
  */
bool breakDetected();

/**
  * Send break to main thread.
  *
  * @param quiet true to print no break message in breakDetected(), false otherwise (default).
  */
void sendBreak(const bool quiet);


#ifdef __cplusplus
}
#endif


#endif
