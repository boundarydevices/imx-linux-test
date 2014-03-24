#!/bin/bash
# demo for dbg_monitor module

# reset and ungate dbg monitor;
/unit_tests/memtool 0x21cc000=0;

# monitor all address range;
/unit_tests/memtool 0x21cc000=0x1;

# enable wdog reset;
/unit_tests/wdt_driver_test.out 5 1 0 &

# enable wdog irq to trigger dbg monitor;
/unit_tests/memtool -16 0x20bc006=0x8001;

# after system hang and wdog auto reset, read address
# of 0x21cc060, 0x21cc070, 0x21cc080 to get the last
# axi address, data, master ID, and status.

