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
 * Copyright (C) 2002-2024 by Thomas Dreibholz
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

#ifndef RSERPOOLMESSAGECREATOR_H
#define RSERPOOLMESSAGECREATOR_H

#include "tdtypes.h"
#include "rserpoolmessage.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
  * Create packet from RSerPoolMessage structure.
  *
  * @param message RSerPoolMessage.
  * @return Size of packet or 0 in case of error.
  */
size_t rserpoolMessage2Packet(struct RSerPoolMessage* message);


#ifdef __cplusplus
}
#endif

#endif
