#!/bin/bash

if ! mount|grep -sq '/sys/kernel/debug'; then
	mount -t debugfs none /sys/kernel/debug
fi

saved_path=$PWD

printf "%-24s %-24s %3s %9s\n" "clock" "parent" "use" "rate"

for foo in $(find /sys/kernel/debug/clock -type d); do
    if [ "$foo" = '/sys/kernel/debug/clock' ]; then
        continue
    fi

    cd $foo

    use="$(cat usecount)"
    rate="$(cat rate)"

    clk="$(basename $foo)"
    cd ..
    parent="$(basename $PWD)"

    if [ "$parent" = 'clock' ]; then
        parent="   ---"
    fi

    printf "%-24s %-24s %2d %10d\n" "$clk" "$parent" "$use" "$rate"

    cd $saved_path
done
