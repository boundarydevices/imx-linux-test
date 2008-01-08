*******************************************************
*******    Test Application for SPI driver    *********
*******************************************************
*    This test send up to 32 bytes to a specific SPI  *
*    device. SPI writes data received data from the   *
*    user into Tx FIFO and waits for the data in the  *
*    Rx FIFO. Once the data is ready in the Rx FIFO,  *
*    it is read and sent to user. Test is considered  *
*    successful if data received = data sent.         *
*                                                     *
*    NOTE: As this test is intended to test the SPI   *
*    device,it is always configured in loopback mode  *
*    reasons is that it is dangerous to send ramdom   *
*    data to SPI slave devices.                       *
*                                                     *
*    To run the application                           *
*    Options : ./mxc_spi_test1.out
                  <spi_no> <nb_bytes> <value>         *
*                                                     *
*    <spi_no> - CSPI Module number in [0, 1, 2]       *
*    <nb_bytes> - No. of bytes: [1-32]                *
*    <value> - Actual value to be sent                *
*******************************************************
Examples:
./mxc_spi_test1.out  1 0 4 E6E0 
./mxc_spi_test1.out  1 1 16 E6E00001E6E00000

Important Note:
1. For the Legacy SPI driver <nb_bytes> has to be multiple
   of 4 in the specified range whereas this restriction
   is not there for Community SPI framework.
