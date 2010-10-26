*******************************************************
*******    Test Application for IIM driver    *********
*******************************************************
*    This test can read iim value from a bank or      *
*    fuse a value to a bank                           *
*                                                     *
*    Options :
	Read from fuse:
		./mxc_iim_test.out
                 read [-d bank_addr]
		./mxc_iim_test.out
		 fuse [-d bank_addr] [-v value]
*                                                     *
*    <bank_addr> - bank address in fuse map file      *
*    <value> - Value to be writen to a bank.          *
*    read - Indicate that this is a read operation.     *
*    fuse - Indicate that this is a write operation.    *
*******************************************************

Examples:
./mxc_iim_test.out read -d 0xc30
./mxc_iim_test.out fuse 0xc30 -v 0x40

