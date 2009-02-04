#!/bin/sh
PATH=$PATH:/unit_tests/modules:$PWD
source /unit_tests/test-utils.sh

mod=dryice_test
loc=/lib/modules/$(kernel_version)/test/$mod.ko

# Initialize counters
n_passed=0
n_failed=0
n_total=0

################################################################
# fn run()
################################################################
##
## Run the test and increment the results
##
## Arguments: TestType
##
run()
{
	insmod_test $loc $1 $2
	if [ $? -eq 0 ]; then
		rmmod $mod
		n_passed=$(($n_passed+1))
		echo "SUCCESS: $1 $2"
	else
		echo "FAILED: $1 $2"
		n_failed=$(($n_failed+1))
	fi
	n_total=$(($n_total+1))
}
	
run test=setp nr=0
run test=setp nr=1
run test=setp nr=2
#run test=setp nr=3	(destructive test, run manually)
#run test=setp nr=4	(destructive test, run manually)

run test=relp nr=0

run test=getp nr=0

#run test=setr nr=0	(destructive test, run manually)
run test=setr nr=1
#run test=setr nr=2	(destructive test, run manually)
#run test=setr nr=3	(destructive test, run manually)
#run test=setr nr=4	(destructive test, run manually)

run test=sel nr=0
run test=sel nr=1
run test=sel nr=2
#run test=sel nr=3	(destructive test, run manually)
#run test=sel nr=4	(destructive test, run manually)

run test=rel nr=0

run test=chk nr=0
run test=chk nr=1
run test=chk nr=2

#run test=tamp nr=0	(destructive test, run manually)

run test=busy nr=0

echo "SUMMARY:"
echo "passed=$n_passed"
echo "failed=$n_failed"
echo "total =$n_total"

exit $n_failed
