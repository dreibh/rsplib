/*
 *  $Id: localaddresses.h,v 1.2 2004/07/29 15:10:33 dreibh Exp $
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
 * Purpose: Obtaining all Local Addresses
 *
 */


#ifndef LOCALADDRESSES_H
#define LOCALADDRESSES_H


#include "tdtypes.h"
#include "netutilities.h"


#ifdef __cplusplus
extern "C" {
#endif



enum AddressScopingFlags {
   ASF_HideLoopback           = (1 << 0),
   ASF_HideLinkLocal          = (1 << 1),
   ASF_HideSiteLocal          = (1 << 2),
   ASF_HideLocal              = ASF_HideLoopback|ASF_HideLinkLocal|ASF_HideSiteLocal,
   ASF_HideAnycast            = (1 << 3),
   ASF_HideMulticast          = (1 << 4),
   ASF_HideBroadcast          = (1 << 5),
   ASF_HideReserved           = (1 << 6),
   ASF_Default                = ASF_HideBroadcast|ASF_HideMulticast|ASF_HideAnycast|ASF_HideReserved,
   ASF_HideAllExceptLoopback  = (1 << 7),
   ASF_HideAllExceptLinkLocal = (1 << 8),
   ASF_HideAllExceptSiteLocal = (1 << 9)
};



/**
  * Gather local addresses. The obtained address array will be stored
  * to the variable addressArray. It has to be freed using free().
  *
  * @param addressArray Reference to store address array to.
  * @param addresses Reference to store number of addresses to.
  * @param flags Flags.
  * @return true for success; false otherwise.
  */
bool gatherLocalAddresses(union sockaddr_union** addressArray,
                          size_t*                addresses,
                          const unsigned int     flags);



#ifdef __cplusplus
}
#endif

#endif
