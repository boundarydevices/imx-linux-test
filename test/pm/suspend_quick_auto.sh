#!/bin/sh
# This will only run the quickhit tests.

i=0;
while [ "$i" -lt 20000 ];
do
   /unit_tests/rtcwakeup.out -d rtc0 -m mem -s 2;
   i=`expr $i + 1`;
   echo "==============================="
   echo  suspend $i times
   echo "==============================="
done

