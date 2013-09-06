#!/bin/bash

if [ $# -gt 0 ]; then
	echo "Usage:"
	echo "	1. run '$0 > d.txt' on Target Board"
	echo "	2. run 'dot -Tpng -O d.txt' on Host PC to generate a PNG file."
	exit 0
fi

if ! mount|grep -sq '/sys/kernel/debug'; then
	mount -t debugfs none /sys/kernel/debug
fi

saved_path=$PWD

printf "digraph clocktree {\n"
printf "rankdir = LR;\n"
printf "node [shape=record];\n"

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

    if [ "$enable_count" = 0 ]; then
    	bgcolor="red"
    else
    	bgcolor="green"
    fi

    printf "%s [label=\"%s|rate %d|en_cnt %d|pr_cnt %d|flags %08x\" color=%s];\n" \
	"$clk" "$clk" "$rate" "$enable_count" "$prepare_count" "$flags" $bgcolor

    if [ "$parent" = 'clk' ]; then
        parent="virtual_Root"
# 	printf "%s d];\n"  "$clk" "$clk" "$rate"
    else
    	printf "%s -> %s\n" "$parent" "$clk"
    fi

#    printf "%s [label=%s_%d];\n" "$clk" "$rate"
#    printf "%s -> %s ;\n" "$parent" "$clk"

    cd $saved_path
done

printf "}\n"
