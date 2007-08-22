#!/bin/bash

echo "Executing script with arguments A1=$1, A2=$2 ..."

x=0 ; while [ $x -lt 10 ] ; do
   echo "x=$x"
   sleep 1
   let x=$x+1
done
