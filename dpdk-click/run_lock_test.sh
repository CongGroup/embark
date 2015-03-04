#!/bin/bash

CORES="3"
LOCKS="0"
ELTS="8"

for core in $CORES
do
  for lock in $LOCKS
  do
    for elt in $ELTS 
    do
      echo "Lock test for $core cores and $lock locks ($elt elts)"
      ./generate_schedulable_lock_test.rb -e $elt -l $lock -c $core >tmp.click
      outfile=locklog.e.$elt.l.$lock.c.$core.out
      sudo ./userlevel/click -j $core -f tmp.click >$outfile & 
      last=$!
      echo "Running with pid $last"
      sleep 60
      sudo kill $last
      echo "PPS/Core:"
      sudo cat $outfile | grep -v "done 0 " | grep 1s | egrep "[0-9]{5}" | cut -d " " -f3 | /home/justine/tools/summarize_list_of_numbers.rb
      echo
      echo
    done
  done
done

