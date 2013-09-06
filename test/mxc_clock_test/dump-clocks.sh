#!/bin/bash

if ! mount|grep -sq '/sys/kernel/debug'; then
	mount -t debugfs none /sys/kernel/debug
fi

saved_path=$PWD

printf "%-24s %-24s %8s %6s %6s %9s\n" "clock" "parent" "flags" "en_cnt" "pre_cnt" "rate"

for foo in $(find /sys/kernel/debug/clk -type d); do
    if [ "$foo" = '/sys/kernel/debug/clk' ]; then
        continue
    fi

    cd $foo

    if [ -f clk_flags ]; then
        flags="$(cat clk_flags)"
    fi

    if [ -f clk_enable_count ]; then
        enable_count="$(cat clk_enable_count)"
    fi

    if [ -f clk_prepare_count ]; then
        prepare_count="$(cat clk_enable_count)"
    fi

    if [ -f clk_rate ]; then
        rate="$(cat clk_rate)"
    fi

    clk="$(basename $foo)"
    cd ..
    parent="$(basename $PWD)"

    if [ "$parent" = 'clk' ]; then
        parent="   ---"
    fi

    printf "%-24s %-24s %08x %6d %6d %10d\n" "$clk" "$parent" "$flags" "$enable_count" "$prepare_count" "$rate"

    cd $saved_path
done
