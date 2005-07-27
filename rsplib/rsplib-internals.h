/*
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
 * Purpose: RSerPool Library
 *
 */

#ifndef RSPLIBINTERNALS_H
#define RSPLIBINTERNALS_H


#include "rsplib.h"
#include "componentstatusprotocol.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
  * Get component status. The resulting status is dynamically allocated and has to
  * be freed by the caller!
  *
  * @param caeArray Reference to store pointer to resulting ComponentAssociationEntry array to.
  * @param statusText Reference of buffer to store status text to.
  * @param componentAddress Reference of buffer to store component address to.
  * @return Number of ComponentAssociationEntries created.
*/
size_t rspGetComponentStatus(struct ComponentAssociationEntry** caeArray,
                             char*                              statusText,
                             char*                              componentAddress);


#ifdef __cplusplus
}
#endif

#endif
