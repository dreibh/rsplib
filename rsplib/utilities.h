/*
 *  $Id: utilities.h,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Utilities
 *
 */


#ifndef UTILITIES_H
#define UTILITIES_H


#include "tdtypes.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif



/**
  * Get current time: Microseconds since 01 January, 1970.
  *
  * @return Current time.
  */
card64 getMicroTime();

/**
  * Print time stamp.
  *
  * @param fd File to print timestamp to (e.g. stdout, stderr, ...).
  */
void printTimeStamp(FILE* fd);

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
bool safestrcpy(char* dest, const char* src, const size_t size);

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
bool safestrcat(char* dest, const char* src, const size_t size);

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
bool getNextWord(const char* input, char* buffer, const size_t bufferSize, size_t* position);

/**
  * File descriptor comparision pointer function.
  *
  * @param a Pointer to file descriptor 1.
  * @param b Pointer to file descriptor 2.
  * @return Comparision result.
  */
gint fdCompareFunc(gconstpointer a,
                   gconstpointer b);

/**
  * Get first available key/value pair from tree.
  *
  * @param tree GTree.
  * @param key Reference to store found key.
  * @param value Reference to store found value.
  * @return true if key/value pair has been found; false if tree is empty.
  */
bool getFirstTreeElement(GTree* tree, gpointer* key, gpointer* value);


#ifdef __cplusplus
}
#endif


#endif
