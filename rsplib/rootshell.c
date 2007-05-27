/*
 * Execute program as superuser.
 * Copyright (C) 2005-2007 by Thomas Dreibholz
 *
 * $Id$
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char** argv)
{
   if(argc < 2) {
      fprintf(stderr, "Usage: %s [Program] {Argument} ...\n", argv[0]);
      exit(1);
   }

   if( (setuid(0) == 0) && (setgid(0) == 0) ) {
      execve(argv[1], &argv[1], NULL);
      perror("Unable to start program");
   }
   else {
      perror("Unable obtain root permissions. Ensure that rootshell is owned by root and has setuid set!");
   }
   return(1);
}
