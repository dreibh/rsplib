/*
 *  $Id: servertable.h,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Server Table
 *
 */


#ifndef SERVERTABLE_H
#define SERVERTABLE_H


#include "tdtypes.h"
#include "tagitem.h"
#include "dispatcher.h"
#include "timer.h"
#include "serverannounce.h"
#include "asapmessage.h"


#ifdef __cplusplus
extern "C" {
#endif


struct ServerTable
{
   struct Dispatcher* Dispatcher;
   GList*             NameServerAnnounceList;
   card64             NameServerAnnounceListAddition;
   int                NameServerIPv4AnnounceSocket;
#ifdef HAVE_IPV6
   int                NameServerIPv6AnnounceSocket;
#endif
   struct Timer*      NameServerAnnounceMaintenanceTimer;

   card64             NameServerAnnounceMaintenanceInterval;
   card64             NameServerAnnounceTimeout;
   cardinal           NameServerConnectMaxTrials;
   card64             NameServerConnectTimeout;
};



/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param tags TagItem array.
  */
struct ServerTable* serverTableNew(struct Dispatcher* dispatcher,
                                   struct TagItem*    tags);


/**
  * Destructor.
  *
  * @param serverTable ServerTable.
  */
void serverTableDelete(struct ServerTable* ServerTable);

/**
  * Do server hunt.
  *
  * @param serverTable ServerTable.
  * @param protocol Desired nameserver protocol (SCTP or TCP).
  * @return Number of valid servers seen.
  */
unsigned int serverTableFindServer(struct ServerTable* serverTable,
                                   const int           protocol);


#ifdef __cplusplus
}
#endif


#endif
