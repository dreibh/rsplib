/*
 *  $Id: utilities.c,v 1.1 2004/07/18 15:30:43 dreibh Exp $
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


#include "tdtypes.h"
#include "loglevel.h"
#include "utilities.h"

#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <sched.h>
#include <time.h>

#include <ext_socket.h>
#include <sys/uio.h>
#include <glib.h>


/* ###### File descriptor pointer comparision function ################### */
gint fdCompareFunc(gconstpointer a,
                   gconstpointer b)
{
   const int f1 = (int)a;
   const int f2 = (int)b;

   if(f1 < f2) {
      return(-1);
   }
   else if(f1 > f2) {
      return(1);
   }
   return(0);
}


/* ###### GTree traverse function to get the first key/value pair ######## */
static gint getFirstTreeElementTraverseFunc(gpointer key, gpointer value, gpointer data)
{
   ((gpointer*)data)[0] = key;
   ((gpointer*)data)[1] = value;
   return(true);
}


/* ###### Remove first key/value pair from tree and return value ######### */
bool getFirstTreeElement(GTree* tree, gpointer* key, gpointer* value)
{
   gpointer data[2] = { NULL, NULL };

   g_tree_traverse(tree,getFirstTreeElementTraverseFunc,G_IN_ORDER,&data);
   if(data[0] != NULL) {
      *key   = data[0];
      *value = data[1];
      return(true);
   }
   *key   = NULL;
   *value = NULL;
   return(false);
}
