/*
 *  $Id: sockaddrunion.h,v 1.5 2005/07/27 10:26:18 dreibh Exp $
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
 * Purpose: Network Utilities
 *
 */

#ifndef SOCKADDRUNION_H
#define SOCKADDRUNION_H

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#ifdef HAVE_TEST

#define AF_TEST AF_MAX

struct sockaddr_testaddr {
   struct sockaddr _sockaddr;
   unsigned int     ta_addr;
   uint16_t         ta_port;
   double           ta_pos_x;
   double           ta_pos_y;
   double           ta_pos_z;
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
