/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2007 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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

#ifndef RSPUTILITIES_H
#define RSPUTILITIES_H

#include "tdtypes.h"
#include "rserpool.h"
#ifdef ENABLE_CSP
#include "componentstatusreporter.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


int addStaticRegistrar(struct rsp_info* info,
                       const char*      addressString);
void freeStaticRegistrars(struct rsp_info* info);

#ifdef ENABLE_CSP
bool initComponentStatusReporter(struct rsp_info* info,
                                 const char*      parameter);
#endif


#ifdef __cplusplus
}
#endif

#endif
