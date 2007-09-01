#!/bin/bash
# $Id$
# --------------------------------------------------------------------------
#
#              //===//   //=====   //===//   //       //   //===//
#             //    //  //        //    //  //       //   //    //
#            //===//   //=====   //===//   //       //   //===<<
#           //   \\         //  //        //       //   //    //
#          //     \\  =====//  //        //=====  //   //===//    Version II
#
# ------------- An Efficient RSerPool Prototype Implementation -------------
#
# Copyright (C) 2002-2007 by Thomas Dreibholz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: dreibh@iem.uni-due.de


# ====== Preparation ========================================================
if [ $# -lt 4 ] ; then
   echo "Usage: scriptingservice.sh [run|status] [Directory] [Input] [Output] [Status]"
   exit 1
fi

MODE="$1"
DIRECTORY="$2"
INPUT_NAME="$3"
OUTPUT_NAME="$4"
STATUS_NAME="$5"

cd $DIRECTORY || exit 1


# ====== Work mode ==========================================================
if [ "$MODE" = "run" ] ; then

   # ====== Extract input ===================================================
   tar xzf $INPUT_NAME || exit 1


   # ====== Do the actual work ==============================================
   x=0 ; while [ $x -lt 3 ] ; do
      echo "x=$x $DIRECTORY"
      sleep 1
      let x=$x+1
   done

   ls -l /tmp >a
   cp a b
   cp b c


   # ====== Archive output ==================================================
   tar czf $OUTPUT_NAME a b c


# ====== Obtain status ======================================================
elif [ "$MODE" = "status" ] ; then

   echo "???" >$STATUS_NAME


# ====== Bad mode ===========================================================
else
   echo "ERROR: Bad mode setting $MODE!"
   exit 1
fi

exit 0
