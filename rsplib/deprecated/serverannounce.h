/*
 *  $Id: serverannounce.h,v 1.1 2004/07/18 15:30:43 dreibh Exp $
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
 * Purpose: Server Announce
 *
 */


#ifndef SERVERANNOUNCE_H
#define SERVERANNOUNCE_H


#include "tdtypes.h"
#include "transportaddressblock.h"


#ifdef __cplusplus
extern "C" {
#endif


#define SIF_DYNAMIC (1 << 0)


struct ServerAnnounce
{
   card64       LastUpdate;
   GList*       TransportAddressList;
   unsigned int Flags;
};



/**
  * Constructor.
  *
  * @param transportAddressList Transport address list.
  * @param flags Flags.
  * @return ServerAnnounce or NULL in case of error.
  */
struct ServerAnnounce* serverAnnounceNew(GList*             transportAddressList,
                                         const unsigned int flags);

/**
  * Destructor.
  *
  * @param ServerAnnounce ServerAnnounce.
  */
void serverAnnounceDelete(struct ServerAnnounce* ServerAnnounce);

/**
  * Duplicate ServerAnnounce.
  *
  * @param ServerAnnounce ServerAnnounce.
  * @return Copy of ServerAnnounce or NULL in case of error.
  */
struct ServerAnnounce* serverAnnounceDuplicate(struct ServerAnnounce* ServerAnnounce);

/**
  * Print ServerAnnounce.
  *
  * @param ServerAnnounce ServerAnnounce.
  * @param fd File to write ServerAnnounce to (e.g. stdout, stderr, ...).
  */
void serverAnnouncePrint(struct ServerAnnounce* ServerAnnounce, FILE* fd);

/**
  * ServerAnnounce comparision function.
  *
  * @param a ServerAnnounce 1.
  * @param a ServerAnnounce 2.
  * @return Comparision result.
  */
gint serverAnnounceCompareFunc(gconstpointer a, gconstpointer b);



#ifdef __cplusplus
}
#endif


#endif
