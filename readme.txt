
	Misc Stuff for Linux
	====================

This "misc" folder contains the source code and necessary build files for Linux
utility programs such as tests, demos, etc. All the non-Linux BSP source should
go into this directory.


1. Directory structure
======================

Currently the "misc" directory is organized as follows to cover three types of
programs: bootloader, test and demo.

misc
  |- include
  |- platform
  |- bootloader
  |- lib
  |- module_test
  |_ test
       |_ demo


misc -> include
---------------
	This directory's path is included in the build system so that generic
	header files can be put under this directory and be included by the source
	code.

misc -> platform
----------------
	This directory contains the build output files. Once "make" finishes, a
	platform specific directory will be created.

misc -> test
------------
	The unit test code goes in directories below misc/test.


2. Build Steps
==============

To build all the tests and bootloaders, run make as shown:

make PLATFORM=MXC27530EVB LINUXPATH=/home/marsha/LINUX2.6/linux \
KBUILD_OUTPUT=/home/marsha/LINUX2.6/kbuild/mxc27530evb \
CROSS_COMPILE=/opt/montavista/mobilinux/devkit/arm/v6_vfp_le/bin/arm_v6_vfp_le-

Note that you have to specify 4 things: PLATFORM, LINUXPATH, KBUILD_OUTPUT, and CROSS_COMPILE.

The build results will end up in the platform directory.

To build a single test add the test dir name to the above like:

make PLATFORM=MXC27530EVB LINUXPATH=/home/marsha/LINUX2.6/linux \
KBUILD_OUTPUT=/home/marsha/LINUX2.6/kbuild/mxc27530evb \
CROSS_COMPILE=/opt/montavista/mobilinux/devkit/arm/v6_vfp_le/bin/arm_v6_vfp_le- \
mxc_mu_test


3. Adding new programs
======================

To add new programs such as test and demo, the easiest way is to use other existing
code as an example.  Be careful not to use absolute paths when including files from
the linux tree.


4. Adding autorun scripts
=========================

The autorun scripts are used to run the unit tests each night by the nightly build,
without human interaction.  After the tests are run, a script parses the output
into some emails to let the team know how well testing went that night and which
tests are having difficulty.  These scripts can also make it easier for people who
are not familiar with the unit tests to run them.  Currently the simplest autorun
script is misc/test/wdog/autorun-wdog.sh, which is quoted below.

      #!/bin/sh

      source /unit_tests/test-utils.sh

      #
      # Exit status is 0 for PASS, nonzero for FAIL
      #
      STATUS=0

      check_devnode "/dev/misc/watchdog"

      print_status
      exit $STATUS

All scripts should have this form and contain basically everything that this test
have except for the "check_devnode" line.  Any tests deviating from this may break
the autorun or have output that can't properly be parsed by the nightly build scripts.

Tests must initialize STATUS=0 early on and can never set it to 0 again later in
the script as that might clear a failure code.

There are three files that you should be aware of, autorun.sh which runs the
autorun scripts, autorun-suite.txt which lists the which autoruns to run (and
assigns a test id to each), and test-utils.sh which has functions each autorun
script uses.  All these are checked in to LINUX2.6/misc/.

Any tests that require a specialized kernel build of course can not be run during
the nightly run so they are not listed in autorun-suite.txt.  Still, these can
be useful for humans running tests as they can run the test by hand without
having to be familiar with the unit test.

Also check the misc/test/wdog/Makefile for how the test gets copied.  The script
just has to be added to the the OBJ list since there is a rule for .sh files in
misc/tests/make.rules.

If there are platforms that a test should not be built for or run on, add that
platform to the Makefile's exclude list.

If a autorun needs to run tests differently conditioned on what platform we are
on, there is a function in test-utils.sh for determining platform.
