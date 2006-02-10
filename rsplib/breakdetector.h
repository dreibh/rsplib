/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
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
