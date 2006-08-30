/*
 * The rsplib Prototype -- An RSerPool Implementation.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id$
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <thread.h>
#include <iostream>
#include <signal.h>


using namespace std;


// ###### Signal handler ####################################################
void alarmSignal(int signum)
{
   puts("Timeout!");
   exit(1);
}


// ###### Main program ######################################################
int main(int argc, char** argv)
{
   if(argc != 3) {
      cerr << "Usage: " << argv[0] << " [Seconds] [Command]"
           << endl;
      exit(1);
   }

   signal(SIGALRM, &alarmSignal);
   alarm(atol(argv[1]));
   const int result = system(argv[2]);
   if(result != 0) {
      puts("ERR");
      exit(1);
   }

   return(0);
}
