/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2019 by Thomas Dreibholz
 *
 * Acknowledgements:
 * Realized in co-operation between Siemens AG and
 * University of Essen, Institute of Computer Networking Technology.
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany
 * (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#ifndef REGISTRARTABLE_H
#define REGISTRARTABLE_H


#include "tdtypes.h"
#include "tagitem.h"
#include "sockaddrunion.h"
#include "dispatcher.h"
#include "timer.h"
#include "messagebuffer.h"
#include "poolhandlespacemanagement.h"
#include "rserpoolmessage.h"

#include <ext_socket.h>


#ifdef __cplusplus
extern "C" {
#endif


struct RegistrarTable
{
   struct Dispatcher*                  Dispatcher;

   struct ST_CLASS(PeerListManagement) RegistrarList;
   struct SimpleRedBlackTree           RegistrarAssocIDList;
   int                                 AnnounceSocket;
   union sockaddr_union                AnnounceAddress;
   struct FDCallback                   AnnounceSocketFDCallback;
   unsigned long long                  LastAnnounceHeard;
   size_t                              OutstandingConnects;

   unsigned long long                  RegistrarAnnounceTimeout;
   unsigned long long                  RegistrarConnectTimeout;
   unsigned int                        RegistrarConnectMaxTrials;
};


/**
  * Constructor.
  *
  * @param dispatcher Dispatcher.
  * @param enableAutoConfig true to use multicast announces; false otherwise.
  * @param registrarAnnounceAddress Multicast address for PR announces.
  * @param tags TagItem array.
  */
struct RegistrarTable* registrarTableNew(struct Dispatcher*          dispatcher,
                                         const bool                  enableAutoConfig,
                                         const union sockaddr_union* registrarAnnounceAddress,
                                         struct TagItem*             tags);


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
  * @param transportAddressBlock TransportAddressBlock.
  * @return Error code.
  */
unsigned int registrarTableAddStaticEntry(
                struct RegistrarTable*              registrarTable,
                const struct TransportAddressBlock* transportAddressBlock);

/**
  * Handle notification of registrar hunt socket.
  *
  * @param registrarTable RegistrarTable.
  * @param registrarHuntFD Socket descriptor for registrar hunt socket.
  * @param registrarHuntMessageBuffer MessageBuffer for registrar hunt socket.
  * @param notification Notification to be handled.
  */
void registrarTableHandleNotificationOnRegistrarHuntSocket(struct RegistrarTable*         registrarTable,
                                                           int                            registrarHuntFD,
                                                           struct MessageBuffer*          registrarHuntMessageBuffer,
                                                           const union sctp_notification* notification);

/**
  * Peel off registrar assoc ID from registrar hunt socket.
  *
  * @param registrarTable RegistrarTable.
  * @param registrarHuntFD Socket descriptor for registrar hunt socket.
  * @param registrarHuntMessageBuffer MessageBuffer for registrar hunt socket.
  * @param assocID Association ID to peel off.
  * @return Socket descriptor for peeled-off registrar association.
  */
int registrarTablePeelOffRegistrarAssocID(struct RegistrarTable* registrarTable,
                                          int                    registrarHuntFD,
                                          struct MessageBuffer*  registrarHuntMessageBuffer,
                                          sctp_assoc_t           assocID);

/**
  * Do registrar hunt.
  *
  * @param registrarTable RegistrarTable.
  * @param registrarHuntFD Socket descriptor for registrar hunt socket.
  * @param registrarHuntMessageBuffer MessageBuffer for registrar hunt socket.
  * @param registrarIdentifier Reference to store new PR's identifier to.
  * @return Socket descriptor for peeled-off registrar association.
  */
int registrarTableGetRegistrar(struct RegistrarTable*   registrarTable,
                               int                      registrarHuntFD,
                               struct MessageBuffer*    registrarHuntMessageBuffer,
                               RegistrarIdentifierType* registrarIdentifier);


#ifdef __cplusplus
}
#endif

#endif
