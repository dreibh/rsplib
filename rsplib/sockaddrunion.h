/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
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
