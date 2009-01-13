*******************************************************
*******    Test Application for SPI driver    *********
*******************************************************
*    This test sends bytes of the last parameter to   *
*    a specific SPI device. The maximum transfer      *
*    bytes are 4096 bytes for bits per word less than *
*    8(including 8), 2048 bytes for bits per word     *
*    between 9 and 16, 1024 bytes for bits per word   *
*    larger than 17(including 17). SPI writes data    *
*    received data from the user into Tx FIFO and     *
*    waits for the data in the Rx FIFO. Once the data *
*    is ready in the Rx FIFO, it is read and sent to  *
*    user. Test is considered successful if data      *
*    received = data sent.                            *
*                                                     *
*    NOTE: As this test is intended to test the SPI   *
*    device,it is always configured in loopback mode  *
*    reasons is that it is dangerous to send ramdom   *
*    data to SPI slave devices.                       *
*                                                     *
*    Options : ./mxc_spi_test1.out
                  [-D spi_no]  [-s speed]
                  [-b bits_per_word]
                  [-H] [-O] [-C] <value>
*                                                     *
*    <spi_no> - CSPI Module number in [0, 1, 2]       *
*    <speed> - Max transfer speed                     *
*    <bits_per_word> - bits per word                  *
*    -H - Phase 1 operation of clock                  *
*    -O - Active low polarity of clock                *
*    -C - Active high for chip select                 *
*    <value> - Actual values to be sent               *
*******************************************************

Note:
The spidev driver copies len of bytes to tx buffer, and sends len of units to
SPI device. So it will send much more data to the SPI device when bits per
word is larger than 8, we only care the first len of bytes for this test.

Examples:
./mxc_spi_test1.out -D 0 -s 1000000 -b 8 E6E0
./mxc_spi_test1.out -D 1 -s 1000000 -b 8 -H -O -C E6E0E6E00001E6E00000

