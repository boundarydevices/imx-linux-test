This readme file explains how to build and test the Security (SCC) driver.

Steps to build the Security Test driver Kernel module:
------------------------------------------------------
  1. Go to LINUX2.6/misc/module_test directory.
  2. To clean up the build, do:
        make ARCH=arm CROSS_COMPILE=/usr/local/arm-linux/gcc-3.4.0-glibc-2.3.2/bin/arm-linux- clean
  3. ./configure --with-linux=<PATH TO YOUR LINUX SOURCE TREE>
  4. make ARCH=arm CROSS_COMPILE=/usr/local/arm-linux/gcc-3.4.0-glibc-2.3.2/bin/arm-linux-
  5. mxc_test.ko is found under LINUX2.6/misc/module_test directory.

Steps to build the test application:
------------------------------------
  1. Go to LINUX2.6/misc directory.
  2. Execute '$ make menuconfig' 
  3. Select the "MXC Security Test". This will select the Test application to be
     compiled.
  4. Execute '$ make clean' in the misc directory. This cleans all the
     intermediate files that were generated during the previous
     compilation. (Optional)
  5. Execute '$ make' command in the misc directory.
  6. You can get the .out file in the directory:
     "misc/platform/<platform>/obj/mxc_security/"

Steps to set up the environment:
--------------------------------
  1. After running the Virtio/EVB and booting up the Kernel Image and 
     filesystem, to FTP the Kernel Objects (.ko files) and the application 
     (.out), the ethernet controller needs to be configured as:
     ifconfig <Ethernet interface name> <IP Address>.
  2. Get the Kernel objects and the application by ftp'ing it to the local
     directory.
  3. Exit from the FTP.
  4. To load the Kernel objects, first insert the mxc_scc.ko in the Kernel.
     "insmod mxc_scc.ko"
  5. Next load the Kernel object "mxc_scc_testdrv.ko" in the Kernel.
     "insmod mxc_scc_testdrv.ko"
  6. Check if both modules have got inserted in the Kernel by executing
     "lsmod". This should display both the modules inserted in the Kernel.

Running the application:
------------------------
  1. Execute the application by running the command as
     "./mxc_security_test.out". This will display a list of menu option.*
  2. Click on the appropriate menu selection as desired.**


  *  If the application doesn't execute then the mode needs to be changed by
     executing the command as "chmod +x mxc_security_test.out"
  ** If the Fail State test or Test All functionality is selected, some of the
     functionality like Zeroization cannot be checked next until the reboot
     of Virtio.
