/*
 *  $Id: utilities.h,v 1.2 2004/07/13 14:23:38 dreibh Exp $
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
 * Purpose: Utilities
 *
 */


#ifndef UTILITIES_H
#define UTILITIES_H


#include "tdtypes.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



/**
  * Get current time: Microseconds since 01 January, 1970.
  *
  * @return Current time.
  */
card64 getMicroTime();

/**
  * Print time stamp.
  *
  * @param fd File to print timestamp to (e.g. stdout, stderr, ...).
  */
void printTimeStamp(FILE* fd);

/**
  * File descriptor comparision pointer function.
  *
  * @param a Pointer to file descriptor 1.
  * @param b Pointer to file descriptor 2.
  * @return Comparision result.
  */
gint fdCompareFunc(gconstpointer a,
                   gconstpointer b);

/**
  * Get first available key/value pair from tree.
  *
  * @param tree GTree.
  * @param key Reference to store found key.
  * @param value Reference to store found value.
  * @return true if key/value pair has been found; false if tree is empty.
  */
bool getFirstTreeElement(GTree* tree, gpointer* key, gpointer* value);


#ifdef __cplusplus
}
#endif


#endif
