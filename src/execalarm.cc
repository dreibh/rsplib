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
 * Copyright (C) 2002-2024 by Thomas Dreibholz
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
