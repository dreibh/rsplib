/*
 *  $Id$
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
 * Purpose: ASAP Instance
 *
 */

#ifndef FRACTALGENERATORPACKETS_H
#define FRACTALGENERATORPACKETS_H


#include "netdouble.h"


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


#define FGD_MAX_POINTS 1024

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


#endif
