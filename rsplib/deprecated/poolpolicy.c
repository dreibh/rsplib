/*
 *  $Id: poolpolicy.c,v 1.1 2004/07/13 09:12:09 dreibh Exp $
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
 * Purpose: Pool Policy
 *
 */


#include "tdtypes.h"
#include "poolpolicy.h"



/* ###### Constructor #################################################### */
struct PoolPolicy* poolPolicyNew(const uint8_t type)
{
   struct PoolPolicy* poolPolicy = (struct PoolPolicy*)malloc(sizeof(struct PoolPolicy));
   if(poolPolicy != NULL) {
      memset(poolPolicy,0,sizeof(struct PoolPolicy));
      poolPolicy->Type = type;
   }
   return(poolPolicy);
}


/* ###### Destructor ##################################################### */
void poolPolicyDelete(struct PoolPolicy* poolPolicy)
{
   if(poolPolicy != NULL) {
      free(poolPolicy);
   }
}


/* ###### Duplicate PoolPolicy ########################################### */
struct PoolPolicy* poolPolicyDuplicate(const struct PoolPolicy* source)
{
   struct PoolPolicy* copy = NULL;
   if(source != NULL) {
      copy = (struct PoolPolicy*)malloc(sizeof(struct PoolPolicy));
      if(copy != NULL) {
         memcpy((char*)copy,(char*)source,sizeof(struct PoolPolicy));
      }
   }
   return(copy);
}


/* ###### Print PoolPolicy ############################################### */
void poolPolicyPrint(const struct PoolPolicy* poolPolicy, FILE* fd)
{
   bool printLoad   = false;
   bool printWeight = false;

   if(poolPolicy != NULL) {
      fprintf(fd,"   Policy        = ");
      switch(poolPolicy->Type) {
         case PPT_ROUNDROBIN:
            fprintf(fd,"Round-Robin");
          break;
         case PPT_WEIGHTED_ROUNDROBIN:
            fprintf(fd,"Weighted Round-Robin");
            printWeight = true;
          break;
         case PPT_LEASTUSED:
            fprintf(fd,"Least-Used");
            printLoad = true;
          break;
         case PPT_LEASTUSED_DEGRADATION:
            fprintf(fd,"Least-Used Degradation");
            printLoad = true;
          break;
         case PPT_RANDOM:
            fprintf(fd,"Random");
          break;
         case PPT_WEIGHTED_RANDOM:
            fprintf(fd,"Weighted Random");
            printWeight = true;
          break;
         default:
            fprintf(fd,"$%02x",poolPolicy->Type);
            printLoad   = true;
            printWeight = true;
          break;
      }
      fprintf(fd,"\n");

      if(printLoad) {
         fprintf(fd,"   Load          = %d\n",    poolPolicy->Load);
      }
      if(printWeight) {
         fprintf(fd,"   Weight        = %d\n",    poolPolicy->Weight);
      }
   }
   else {
      fprintf(fd,"   (null policy)\n");
   }
}


/* ###### Try to adapt PoolPolicy ######################################## */
bool poolPolicyAdapt(struct PoolPolicy* poolPolicy, const struct PoolPolicy* source)
{
   switch(source->Type) {
      case PPT_RANDOM:
      case PPT_ROUNDROBIN:
         poolPolicy->Type = source->Type;
         return(true);
       break;
      case PPT_WEIGHTED_RANDOM:
      case PPT_WEIGHTED_ROUNDROBIN:
         if((source->Type == PPT_WEIGHTED_RANDOM) ||
            (source->Type == PPT_WEIGHTED_ROUNDROBIN)) {
            poolPolicy->Type = source->Type;
            return(true);
         }
       break;
      case PPT_LEASTUSED:
      case PPT_LEASTUSED_DEGRADATION:
         if((source->Type == PPT_LEASTUSED) ||
            (source->Type == PPT_LEASTUSED_DEGRADATION)) {
            poolPolicy->Type = source->Type;
            return(true);
         }
       break;
   }
   return(false);
}
