#!/bin/bash

if ! mount|grep -sq '/sys/kernel/debug'; then
	mount -t debugfs none /sys/kernel/debug
fi

saved_path=$PWD

#printf "%-24s %-24s %3s %9s\n" "clock" "parent" "use" "rate"
printf "digraph clocktree {\n"
printf "rankdir = LR;\n"
printf "node [shape=record];\n"

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

    if [ "$use" = 0 ]; then
    	bgcolor="red"
    else
    	bgcolor="green"
    fi

    printf "%s [label=\"%s|rate %d|use $use\" color=%s];\n" "$clk" "$clk" "$rate" $bgcolor

    if [ "$parent" = 'clock' ]; then
        parent="virtual_Root"
# 	printf "%s d];\n"  "$clk" "$clk" "$rate"
    else
    	printf "%s -> %s\n" "$parent" "$clk"
    fi

#    printf "%s [label=%s_%d];\n" "$clk" "$rate"
#    printf "%s -> %s ;\n" "$parent" "$clk"
# "$use" "$rate"

    cd $saved_path
done

printf "}\n"
