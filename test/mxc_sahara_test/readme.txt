BUILDING TESTS ON USER MODE API:
--------------------------------

This directory contains a makefile that builds the program 'apitest' in this
directory. To build 'apitest' program type 'make'. This assumes that CROSS_COMPILE
is properly defined and that the kernel is located at ../../../linux.

Usuage of 'apitest' program:
----------------------------
The program 'apitest' in this will run tests suites on hashing, symmetric crypto, and random number generation.  It can be given an argument to cause it to do otherthan the default of all three.  This looks like:
   apitest -T<testcodes>

   where <testcodes> are any combination of h, s, and r for hashing,
   symmetric, and random, respectively.

   So:
     apitest -Tsss

     will run the symmetric suite three times in a row.

SETTING UP /dev/node FILES
--------------------------
The driver(s) do not (currently) use devfs or any other means to
automatically provide /dev/node files.

After building (with drivers built-in to the kernel, not as modules)
and booting once, the major numbers of the driver(s) can be determined
by looking at /proc/devices, and the appropriate nodes can then be
created in the filesystem.  If using loadable module(s), the module
must first be loaded.

In order to make the devnode file for user programs, you will need to
know the driver's major number.  Look for the driver's name, sahara,
in /proc/devices.

  mknod /dev/sahara c <major-node> 0
  
  Or, after the driver is in operation,  just execute this command:
    mknod /dev/sahara c `grep sahara  /proc/devices | sed -e 's/ .*//'` 0
    
    In order to make the devnode for the kernel tester, you will need to
    know the driver's major number.  Look for the driver's name,
    FSL_API_TEST, in /proc/devices.
    
   mknod /dev/shwtest c <major-node> 0
      
  Or, after the driver is in operation, just execute this command:
    mknod /dev/shwtest c `grep FSL_API_TEST  /proc/devices | sed -e 's/
      .*//'` 0     
