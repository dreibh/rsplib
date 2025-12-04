/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2026 by Thomas Dreibholz
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

#ifndef SOCKADDRUNION_H
#define SOCKADDRUNION_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#ifdef HAVE_TEST

#define AF_TEST AF_MAX

struct sockaddr_testaddr {
   struct sockaddr _sockaddr;
   unsigned int     ta_addr;
   uint16_t         ta_port;
};

#define ta_family _sockaddr.sa_family

#endif


union sockaddr_union {
   struct sockaddr          sa;
   struct sockaddr_in       in;
   struct sockaddr_in6      in6;
#ifdef HAVE_TEST
   struct sockaddr_testaddr ta;
#endif
};


#endif
