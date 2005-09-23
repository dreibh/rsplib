/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id: cspmonitor.c 0 2005-03-02 13:34:16Z dreibh $
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

#ifndef CALCAPPPACKETS_H
#define CALCAPPPACKETS_H


#define CALCAPP_REQUEST       1
#define CALCAPP_ACCEPT        2
#define CALCAPP_REJECT        3
#define CALCAPP_ABORT         4
#define CALCAPP_COMPLETE      5
#define CALCAPP_KEEPALIVE     6
#define CALCAPP_KEEPALIVE_ACK 7

struct CalcAppMessage
{
   uint32_t Type;
   uint32_t JobID;
   double   JobSize;
   double   Completed;
};


struct CalcAppCookie
{
   uint32_t JobID;
   double   JobSize;
   double   Completed;
};


#endif
