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

#ifndef POOLHANDLE_H
#define POOLHANDLE_H

#include <ctype.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_POOLHANDLESIZE 32

struct PoolHandle
{
   size_t        Size;
   unsigned char Handle[MAX_POOLHANDLESIZE];
};


void poolHandleNew(struct PoolHandle*   poolHandle,
                   const unsigned char* handle,
                   const size_t         size);
void poolHandleDelete(struct PoolHandle* poolHandle);
void poolHandleGetDescription(const struct PoolHandle* poolHandle,
                              char*                    buffer,
                              const size_t             bufferSize);
void poolHandlePrint(const struct PoolHandle* poolHandle,
                     FILE*                    fd);
int poolHandleComparison(const struct PoolHandle* poolHandle1,
                         const struct PoolHandle* poolHandle2);


#ifdef __cplusplus
}
#endif

#endif
