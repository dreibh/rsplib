/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
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

#include "poolnamespacemanagement-basics.h"
#include "stringutilities.h"
#include "randomizer.h"


const PoolElementSeqNumberType SeqNumberStart = (~0) ^ 0xf;


/* ###### Get textual description of error code ########################## */
const char* poolNamespaceManagementGetErrorDescription(const unsigned int errorCode)
{
   return(rserpoolErrorGetDescription(errorCode));
}


/* ###### Get pool element identifier #################################### */
PoolElementIdentifierType getPoolElementIdentifier()
{
   PoolElementIdentifierType poolElementIdentifier = 1 + (random32() % 0xfffffffe);
   return(poolElementIdentifier);
}
