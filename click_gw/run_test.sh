#!/bin/bash
for i in {1000..10000..1000} 
do
    echo "Rule #: $i"
    head ./conf/fw.rules -n $i > ./conf/test_fw.rules
    ./userlevel/click -h accum.average_time ./conf/test_gw.click
done