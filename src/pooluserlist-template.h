/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2022 by Thomas Dreibholz
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

#ifndef INTERNAL_POOLTEMPLATE
#error Do not include this file directly, use poolhandlespacemanagement.h
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct ST_CLASS(PoolUserList)
{
   struct ST_CLASSNAME PoolUserListStorage;
};


void ST_CLASS(poolUserListNew)(struct ST_CLASS(PoolUserList)* poolUserList);
void ST_CLASS(poolUserListDelete)(struct ST_CLASS(PoolUserList)* poolUserList);
size_t ST_CLASS(poolUserListGetPoolUserNodes)(
          const struct ST_CLASS(PoolUserList)* poolUserList);
struct ST_CLASS(PoolUserNode)* ST_CLASS(poolUserListGetFirstPoolUserNode)(
                                  struct ST_CLASS(PoolUserList)* poolUserList);
struct ST_CLASS(PoolUserNode)* ST_CLASS(poolUserListGetNextPoolUserNode)(
                                  struct ST_CLASS(PoolUserList)* poolUserList,
                                  struct ST_CLASS(PoolUserNode)* poolUserListNode);
void ST_CLASS(poolUserListGetDescription)(struct ST_CLASS(PoolUserList)* poolUserList,
                                          char*                          buffer,
                                          const size_t                   bufferSize);
void ST_CLASS(poolUserListVerify)(struct ST_CLASS(PoolUserList)* poolUserList);
void ST_CLASS(poolUserListPrint)(struct ST_CLASS(PoolUserList)* poolUserList,
                                 FILE*                          fd,
                                 const unsigned int             fields);
void ST_CLASS(poolUserListClear)(struct ST_CLASS(PoolUserList)* poolUserList);
struct ST_CLASS(PoolUserNode)* ST_CLASS(poolUserListAddPoolUserNode)(
                                  struct ST_CLASS(PoolUserList)* poolUserList,
                                  struct ST_CLASS(PoolUserNode)* poolUserListNode);
struct ST_CLASS(PoolUserNode)* ST_CLASS(poolUserListAddOrUpdatePoolUserNode)(
                                  struct ST_CLASS(PoolUserList)*  poolUserList,
                                  struct ST_CLASS(PoolUserNode)** poolUserNode);
struct ST_CLASS(PoolUserNode)* ST_CLASS(poolUserListFindPoolUserNode)(
                                  struct ST_CLASS(PoolUserList)* poolUserList,
                                  const int                      connectionSocketDescriptor,
                                  const sctp_assoc_t             connectionAssocID);
struct ST_CLASS(PoolUserNode)* ST_CLASS(poolUserListRemovePoolUserNode)(
                                  struct ST_CLASS(PoolUserList)* poolUserList,
                                  struct ST_CLASS(PoolUserNode)* poolUserListNode);


#ifdef __cplusplus
}
#endif
