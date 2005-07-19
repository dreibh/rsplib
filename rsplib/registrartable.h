/*
 *  $Id: registrartable.h,v 1.1 2005/07/19 08:46:31 dreibh Exp $
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
 * Purpose: Registrar Table
 *
 */


#ifndef SERVERTABLE_H
#define SERVERTABLE_H


#include "tdtypes.h"
#include "tagitem.h"
#include "sockaddrunion.h"
#include "dispatcher.h"
#include "timer.h"
#include "poolhandlespacemanagement.h"
#include "rserpoolmessage.h"


#ifdef __cplusplus
extern "C" {
#endif


struct RegistrarTable
{
   struct Dispatcher*                  Dispatcher;

   struct ST_CLASS(PeerListManagement) RegistrarList;
   int                                 AnnounceSocket;
   union sockaddr_union                AnnounceAddress;
   struct FDCallback                   AnnounceSocketFDCallback;
   unsigned long long                  LastAnnounceHeard;

   int                                 RegistrarSocket;
   sctp_assoc_t                        RegistrarAssocID;

   unsigned long long                  RegistrarAnnounceTimeout;
   unsigned long long                  RegistrarConnectTimeout;
   unsigned int                        RegistrarConnectMaxTrials;
};


/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param tags TagItem array.
  */
struct RegistrarTable* registrarTableNew(struct Dispatcher* dispatcher,
                                         struct TagItem*    tags);


/**
  * Destructor.
  *
  * @param registrarTable RegistrarTable.
  */
void registrarTableDelete(struct RegistrarTable* RegistrarTable);

/**
  * Add static registrar entry.
  *
  * @param registrarTable RegistrarTable.
  * @param addressArray Addresses.
  * @param addresses Number of addresses.
  * @return Error code.
  */
unsigned int registrarTableAddStaticEntry(struct RegistrarTable*   registrarTable,
                                          union sockaddr_union* addressArray,
                                          size_t                addresses);

/**
  * Do registrar hunt.
  *
  * @param registrarTable RegistrarTable.
  * @param registrarFD Socket description for registrar connection.
  * @param registrarIdentifier Reference to store new NS's identifier to.
  * @return Assoc ID in case of success; false otherwise.
  */
sctp_assoc_t registrarTableGetRegistrar(struct RegistrarTable*   registrarTable,
                                        int                      registrarFD,
                                        RegistrarIdentifierType* registrarIdentifier)


#ifdef __cplusplus
}
#endif

#endif
