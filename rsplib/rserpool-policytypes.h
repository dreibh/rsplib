/*
 * An Efficient RSerPool Pool Handlespace Management Implementation
 * Copyright (C) 2004-2006 by Thomas Dreibholz
 *
 * $Id: poolpolicysettings.h 1091 2006-05-09 08:48:20Z dreibh $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef RSERPOOL_POLICYTYPES_H
#define RSERPOOL_POLICYTYPES_H


#define PPT_UNDEFINED                                 0x00000000

#define PPT_ROUNDROBIN                                0x01000000
#define PPT_WEIGHTED_ROUNDROBIN                       0x02000000
#define PPT_RANDOM                                    0x03000000
#define PPT_WEIGHTED_RANDOM                           0x04000000

#define PPT_LEASTUSED                                 0x05000000
#define PPT_LEASTUSED_DEGRADATION                     0x06000000
#define PPT_RANDOMIZED_LEASTUSED                      0x07000000
#define PPT_RANDOMIZED_LEASTUSED_DEGRADATION          0x08000000

#define PPT_PRIORITY_LEASTUSED                        0x09000000
#define PPT_PRIORITY_LEASTUSED_DEGRADATION            0x0a000000
#define PPT_RANDOMIZED_PRIORITY_LEASTUSED             0x0b000000
#define PPT_RANDOMIZED_PRIORITY_LEASTUSED_DEGRADATION 0x0c000000

#define PPT_WEIGHTED_RANDOM_DPF                       0x20000000
#define PPT_LEASTUSED_DPF                             0x21000000
#define PPT_LEASTUSED_DEGRADATION_DPF                 0x22000000


#define PPT_IS_ADAPTIVE(p) \
   ( ((p) == PPT_LEASTUSED) || \
     ((p) == PPT_LEASTUSED_DPF) || \
     ((p) == PPT_LEASTUSED_DEGRADATION) || \
     ((p) == PPT_LEASTUSED_DEGRADATION_DPF) || \
     ((p) == PPT_PRIORITY_LEASTUSED) || \
     ((p) == PPT_PRIORITY_LEASTUSED_DEGRADATION) || \
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
