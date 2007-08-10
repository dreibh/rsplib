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

#ifndef TAGITEM_H
#define TAGITEM_H

#include <sys/types.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif



#define TAG_DONE   0   /* Terminates array of TagItems.                   */
#define TAG_END    0   /* Synonym for TAG_DONE.                           */
#define TAG_IGNORE 1   /* Ignore this item.                               */
#define TAG_MORE   2   /* Data is pointer to another array of TagItems,
                          current array is terminated.                    */
#define TAG_SKIP   3   /* Skip this and the next Data items.              */
#define TAG_USER   1000000   /* First usable tag for user data.           */



typedef unsigned int  tag_t;
typedef unsigned long tagdata_t;


struct TagItem
{
   tag_t     Tag;
   tagdata_t Data;
};



/**
  * Allocate and clear tag item array. The array has to be freed using
  * tagListFree().
  *
  * @param items Number of tag items to allocate.
  * @return TagItem array or NULL in case of error.
  *
  * @see freeListAllocate
  */
struct TagItem* tagListAllocate(const size_t items);

/**
  * Free tag item array allocated by tagListAllocate().
  *
  * @param tagList TagItem array.
  *
  * @see tagListAllocate
  */
void tagListFree(struct TagItem* tagList);

/**
  * Get size of tag item array (in number of tag items).
  *
  * @param tagList TagItem array.
  * @return Size of tag item array.
  */
size_t tagListGetSize(struct TagItem* tagList);

/**
  * Duplicate tag item array.
  *
  * @param tagList TagItem array.
  * @return TagItem array or NULL in case of error.
  */
struct TagItem* tagListDuplicate(struct TagItem* tagList);

/**
  * Create a new tag item array containing all tag items of
  * the given list which belong to any tag given in the filter
  * array.
  *
  * @param tagList TagItem array.
  * @return TagItem array or NULL in case of error.
  */
struct TagItem* tagListDuplicateFilter(struct TagItem* tagList,
                                       const tag_t*    filterArray);

/**
  * Get next tag item in list.
  *
  * @param tagList TagItem array.
  * @return Next TagItem or NULL if it is last.
  */
struct TagItem* tagListNext(struct TagItem* tagList);

/**
  * Find tag item with given tag in tag list.
  *
  * @param tagList TagItem array.
  * @param tag Tag to search.
  * @return TagItem or NULL if not found.
  */
struct TagItem* tagListFind(struct TagItem* tagList,
                            const tag_t     tag);

/**
  * Get data of given tag in tag list or default value.
  *
  * @param tagList TagItem array.
  * @param tag Tag to search.
  * @param defaultValue Value to return if tag is not found.
  * @return Data of tag item found or given default value when not found.
  */
tagdata_t tagListGetData(struct TagItem* tagList,
                         const tag_t     tag,
                         const tagdata_t defaultValue);

/**
  * Set data of given tag in tag list or default value. If the given
  * tag does not exist, no change is made.
  *
  * @param tagList TagItem array.
  * @param tag Tag to search.
  * @param value Value to set tag's value to.
  * @return TagItem if item has been found; NULL otherwise.
  */
struct TagItem* tagListSetData(struct TagItem* tagList,
                               const tag_t     tag,
                               const tagdata_t value);

/**
  * Print tag list.
  *
  * @param tagList TagItem array.
  * @param fd File to write tag list to (e.g. stdout, stderr, ...).
  */
void tagListPrint(struct TagItem* tagList, FILE* fd);


#ifdef __cplusplus
}
#endif


#endif
