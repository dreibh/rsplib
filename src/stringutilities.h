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

#ifndef STRINGUTILITIES_H
#define STRINGUTILITIES_H

#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
  * strcpy() with length check. The result is truncated if the destination
  * length is not sufficient. It will always be 0-terminated, except for
  * destination size 0.
  *
  * @param dest Destination.
  * @param sec Source.
  * @param size Maximum size of destination.
  * @return true for success; false if result had to be truncated.
  */
int safestrcpy(char* dest, const char* src, const size_t size);

/**
  * strcat() with length check. The result is truncated if the destination
  * length is not sufficient. It will always be 0-terminated, except for
  * destination size 0.
  *
  * @param dest Destination.
  * @param sec Source.
  * @param size Maximum size of destination.
  * @return true for success; false if result had to be truncated.
  */
int safestrcat(char* dest, const char* src, const size_t size);

/**
  * Find first occurrence of character in string.
  *
  * @param string String.
  * @param character Character.
  * @return Position of first occurrence or NULL if not found.
  */
char* strindex(char* string, const char character);

/**
  * Find last occurrence of character in string.
  *
  * @param string String.
  * @param character Character.
  * @return Position of last occurrence or NULL if not found.
  */
char* strrindex(char* string, const char character);

/**
  * Duplicate memory block by allocating a new block and copying the old one.
  *
  * @param source Pointer to memory block to be copied.
  * @param size Size of memory block to be copied.
  * @return New memory block or NULL if allocation failed.
  */
void* memdup(const void* source, const size_t size);

/**
  * Get position of next word in text string.
  *
  * @param input Input string.
  * @param buffer Buffer to copy word to.
  * @param bufferSize Maximum size of buffer.
  * @param position Position to start scanning from. This is a reference variable, the end position of the scanned word will be written to it!
  * @return true for success; false otherwise.
  */
int getNextWord(const char* input, char* buffer, const size_t bufferSize, size_t* position);


#ifdef __cplusplus
}
#endif

#endif
