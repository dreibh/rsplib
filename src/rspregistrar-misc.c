/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2019 by Thomas Dreibholz
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

#include "rspregistrar.h"


/* ###### Hook to be called after successful registration ################ */
void registrarRegistrationHook(struct Registrar*                 registrar,
                               struct ST_CLASS(PoolElementNode)* poolElementNode)
{
 /*
   puts("REGISTRATION:");
   ST_CLASS(poolElementNodePrint)(poolElementNode, stdout, ~0);
   puts("");
*/
}


/* ###### Hook to be called before deregistration ######################## */
void registrarDeregistrationHook(struct Registrar*                 registrar,
                                 struct ST_CLASS(PoolElementNode)* poolElementNode)
{
 /*
   puts("DEREGISTRATION:");
   ST_CLASS(poolElementNodePrint)(poolElementNode, stdout, ~0);
   puts("");
*/
}
