/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


int test() {
  char buf[48];
  int fd;

  fd = open("/dev/sdma_test",O_RDWR);


  if(fd < 0){
    printf("Failed open\n");
    exit(-1);
  }


  write(fd, buf, 0);

  sleep(1);


  read(fd, buf, 0);

  close(fd);
  return 0;
}

int main(int argc,char **argv) {
  test();

  return 0;
}

