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

#ifndef RSERPOOL_POLICYTYPES_H
#define RSERPOOL_POLICYTYPES_H


#define PPT_UNDEFINED                                 0x00000000


#define PPT_ROUNDROBIN                                0x00000001
#define PPT_WEIGHTED_ROUNDROBIN                       0x00000002
#define PPT_RANDOM                                    0x00000003
#define PPT_WEIGHTED_RANDOM                           0x00000004
#define PPT_PRIORITY                                  0x00000005

#define PPT_LEASTUSED                                 0x40000001
#define PPT_LEASTUSED_DEGRADATION                     0x40000002
#define PPT_PRIORITY_LEASTUSED                        0x40000003
#define PPT_RANDOMIZED_LEASTUSED                      0x40000004


#define PPT_RANDOMIZED_PRIORITY_LEASTUSED             0xb0001001
#define PPT_RANDOMIZED_LEASTUSED_DEGRADATION          0xb0001002
#define PPT_PRIORITY_LEASTUSED_DEGRADATION            0xb0001003
#define PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION 0xb0001004

#define PPT_WEIGHTED_RANDOM_DPF                       0xb0002001
#define PPT_LEASTUSED_DPF                             0xb0002002
#define PPT_LEASTUSED_DEGRADATION_DPF                 0xb0002003
#define PPT_PRIORITY_LEASTUSED_DPF                    0xb0002004
#define PPT_PRIORITY_LEASTUSED_DEGRADATION_DPF        0xb0002005


/*
 * NOTE:
 * Make sure to adapt the following macro when adding a new dynamic policy!
 * Otherwise, the PE will not update its status!
 */

#define PPT_IS_ADAPTIVE(p) \
   ( ((p) == PPT_LEASTUSED) || \
     ((p) == PPT_LEASTUSED_DPF) || \
     ((p) == PPT_LEASTUSED_DEGRADATION) || \
     ((p) == PPT_LEASTUSED_DEGRADATION_DPF) || \
     ((p) == PPT_PRIORITY_LEASTUSED) || \
     ((p) == PPT_PRIORITY_LEASTUSED_DEGRADATION) || \
     ((p) == PPT_PRIORITY_LEASTUSED_DPF) || \
     ((p) == PPT_PRIORITY_LEASTUSED_DEGRADATION_DPF) || \
     ((p) == PPT_RANDOMIZED_LEASTUSED) || \
     ((p) == PPT_RANDOMIZED_LEASTUSED_DEGRADATION) || \
     ((p) == PPT_RANDOMIZED_PRIORITY_LEASTUSED) || \
     ((p) == PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION) )


#define PPV_MIN_WEIGHT                    0
#define PPV_MAX_WEIGHT           0xffffffff
#define PPV_MIN_LOAD                      0
#define PPV_MAX_LOAD             0xffffffff
#define PPV_MIN_LOAD_DEGRADATION          0
#define PPV_MAX_LOAD_DEGRADATION 0xffffffff
#define PPV_MIN_LOADDPF                   0
#define PPV_MAX_LOADDPF          0xffffffff
#define PPV_MIN_WEIGHTDPF                 0
#define PPV_MAX_WEIGHTDPF        0xffffffff


#endif
