/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//    Version II
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2007 by Thomas Dreibholz
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

#ifndef FRACTALGENERATORPACKETS_H
#define FRACTALGENERATORPACKETS_H

#include "netdouble.h"

#include <stdint.h>


// Ensure consistent alignment!
#pragma pack(push)
#pragma pack(4)


#define PPID_FGP 0x29097601

#define FGPT_PARAMETER 0x01
#define FGPT_DATA      0x02

struct FGPCommonHeader
{
   uint8_t  Type;
   uint8_t  Flags;
   uint16_t Length;
};



#define FGPA_MANDELBROT  1
#define FGPA_MANDELBROTN 2


struct FGPParameter
{
   FGPCommonHeader  Header;
   uint32_t         Width;
   uint32_t         Height;
   uint32_t         MaxIterations;
   uint32_t         AlgorithmID;
   network_double_t C1Real;
   network_double_t C1Imag;
   network_double_t C2Real;
   network_double_t C2Imag;
   network_double_t N;
};

#define FGD_MAX_POINTS 324

struct FGPData
{
   FGPCommonHeader Header;
   uint32_t        StartX;
   uint32_t        StartY;
   uint32_t        Points;
   uint32_t        Buffer[FGD_MAX_POINTS];
};


/**
  * Calculate size of FGP data message for given amount of points.
  *
  * @param points Number of points.
  * @return Message size.
  */
inline size_t getFGPDataSize(const size_t points)
{
   struct FGPData fgpData;
   return(((long)&fgpData.Buffer - (long)&fgpData) + sizeof(uint32_t) * points);
}



#define FGP_COOKIE_ID "<FG-TD1>"

struct FGPCookie
{
   char         ID[8];
   FGPParameter Parameter;
   uint32_t     CurrentX;
   uint32_t     CurrentY;
};


struct FractalGeneratorStatus {
   struct FGPParameter Parameter;
   uint32_t            CurrentX;
   uint32_t            CurrentY;
};


#pragma pack(pop)

#endif
