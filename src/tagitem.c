/* --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //       //   //===//
 *             //    //  //        //    //  //       //   //    //
 *            //===//   //=====   //===//   //       //   //===<<
 *           //   \\         //  //        //       //   //    //
 *          //     \\  =====//  //        //=====  //   //===//   Version III
 *
 * ------------- An Efficient RSerPool Prototype Implementation -------------
 *
 * Copyright (C) 2002-2023 by Thomas Dreibholz
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
 * Contact: thomas.dreibholz@gmail.com
 */

#include "tdtypes.h"
#include "loglevel.h"
#include "tagitem.h"



/* ###### Allocate tag items ############################################# */
struct TagItem* tagListAllocate(const size_t items)
{
   return((struct TagItem*)calloc(items,sizeof(struct TagItem)));
}


/* ###### Free tag items ################################################# */
void tagListFree(struct TagItem* tagList)
{
   if(tagList != NULL) {
      free(tagList);
   }
}


/* ###### Count tag items ################################################ */
size_t tagListGetSize(struct TagItem* tagList)
{
   size_t count = 0;
   if(tagList != NULL) {
      while(tagList[count].Tag != TAG_DONE) {
         count++;
      }
      return(count + 1);
   }
   return(0);
}


/* ###### Duplicate tag items ############################################ */
struct TagItem* tagListDuplicate(struct TagItem* tagList)
{
   const size_t    count = tagListGetSize(tagList);
   struct TagItem* copy;
   size_t          i;

   if(count > 0) {
      copy = tagListAllocate(count);
      if(copy) {
         for(i = 0;i < count;i++) {
            copy[i].Tag  = tagList[i].Tag;
            copy[i].Data = tagList[i].Data;
         }
      }
      return(copy);
   }
   return(NULL);
}



/* ###### Duplicate tag items with filtering ############################# */
struct TagItem* tagListDuplicateFilter(struct TagItem* tagList,
                                       const tag_t*    filterArray)
{
   const size_t    count = tagListGetSize(tagList);
   struct TagItem* copy;
   size_t          i, j, k;

   if(count > 0) {
      copy = tagListAllocate(count);
      if(copy) {
         LOG_VERBOSE5
         fputs("Filter-copying tag list...\n", stdlog);
         LOG_END
         k = 0;
         for(i = 0, j = 0;i < count;i++) {
            while(filterArray[k] != TAG_DONE) {
               if(tagList[i].Tag == filterArray[k]) {
                  LOG_VERBOSE5
                  fprintf(stdlog,"Copying tag #%u\n", tagList[i].Tag);
                  LOG_END
                  copy[j].Tag  = tagList[i].Tag;
                  copy[j].Data = tagList[i].Data;
                  j++;
                  break;
               }
               k++;
            }
         }
         copy[j++].Tag = TAG_DONE;
      }
      return(copy);
   }
   return(NULL);
}


/* ###### Get next tag item ############################################## */
struct TagItem* tagListNext(struct TagItem* tagList)
{
   struct TagItem* tagListPtr = tagList;
   bool            updated    = false;

   while(true) {
      if(tagListPtr == NULL) {
         return(NULL);
      }
      switch (tagListPtr->Tag) {
         case TAG_MORE:
            if(!(tagListPtr = (struct TagItem *)tagListPtr->Data)) {
               return(NULL);
            }
            continue;
        case TAG_IGNORE:
         break;
        case TAG_END:
           return(NULL);
        case TAG_SKIP:
           tagListPtr += (long)tagListPtr->Data;
         break;
        default:
           if(updated) {
              return(tagListPtr);
           }
         break;
      }

      tagListPtr++;
      updated = true;
   }
}


/* ###### Find tag item ################################################## */
struct TagItem* tagListFind(struct TagItem* tagList, const tag_t tag)
{
   struct TagItem* tagListPtr = tagList;

   LOG_VERBOSE5
   fprintf(stdlog,"Looking for tag #%u...\n",tag);
   LOG_END

   while(tagListPtr != NULL) {
      if(tagListPtr->Tag == tag) {
         LOG_VERBOSE5
         fputs("Tag found\n",stdlog);
         LOG_END
         return(tagListPtr);
      }

      tagListPtr = tagListNext(tagListPtr);
   }

   LOG_VERBOSE5
   fputs("Tag not found\n",stdlog);
   LOG_END
   return(NULL);
}


/* ###### Get tag data ################################################### */
tagdata_t tagListGetData(struct TagItem* tagList,
                         const tag_t     tag,
                         const tagdata_t defaultValue)
{
   struct TagItem* found;

   found = tagListFind(tagList,tag);
   if(found != NULL) {
      LOG_VERBOSE5
      fprintf(stdlog,"Get value %u ($%x) for tag #%u\n",
              (unsigned int)found->Data, (unsigned int)found->Data, tag);
      LOG_END
      return(found->Data);
   }

   LOG_VERBOSE5
   fprintf(stdlog,"Using default value %u ($%x) for tag #%u\n",
           (unsigned int)defaultValue, (unsigned int)defaultValue, tag);
   LOG_END
   return(defaultValue);
}


/* ###### Set tag data ################################################### */
struct TagItem* tagListSetData(struct TagItem* tagList,
                               const tag_t     tag,
                               const tagdata_t value)
{
   struct TagItem* found;

   found = tagListFind(tagList,tag);
   if(found != NULL) {
      LOG_VERBOSE5
      fprintf(stdlog,"Set value %u ($%x) for tag #%u\n",
              (unsigned int)value, (unsigned int)value, tag);
      LOG_END
      found->Data = value;
   }

   return(found);
}


/* ###### Print tag items ################################################ */
void tagListPrint(struct TagItem* tagList, FILE* fd)
{
   struct TagItem* tagListPtr = tagList;
   unsigned int    number  = 1;

   fputs("TagList: ",fd);
   if(tagListPtr != NULL) {
      fputs("\n",fd);
      while(tagListPtr != NULL) {
         fprintf(fd,"   %5d: tag %9d -> %9d ($%08x)\n",
                 number++, tagListPtr->Tag, (unsigned int)tagListPtr->Data, (unsigned int)tagListPtr->Data);
         tagListPtr = tagListNext(tagListPtr);
      }
   }
   else {
      fputs("(empty)\n",fd);
   }
}
