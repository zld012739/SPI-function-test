# SPI-function-test
SPI-function-test
Language:Python and C

Consists of two parts:

1.Host computer script.
2.STM32 lower computer.

This script function include:

1.Send command through UART to STM32, include GUI. 
2.Recieve respone from STM32 through UART and compare the obtained data with the expected test result than print report.

The STM32(SPI MASTER) function:

1.Recieve command from UART than send out to UUT through SPI interface.
2.Recieve respone from UUT through SPI interface than send out through UART.



User guides:

1.run spi_test.py in command line:python spi_test.py.
2.A GUI will open than operate in GUI.
3.Selet serial port number and set buad rate(230400bps, The same setting in STM32) in 'Serial port' drop-down list.
4.Select 'test file'(command send to STM32) and 'verification file'(the expected result to compare with the recieve from STM32) in 'File' drop-down list.
5.Star test.



Test file and verification file specification:
1.The file name must end with 'Cfile.txt'.
2.Only one command can be placed in a line.
3.A command is divided into three sections separated by '#'.Example:default_check1#Test data is transmitted MSB first from the UUT, Read Product ID register (0x56)#560A
4.First section means Command type(used by script,map to the way of deal the result), second section is the description of command and the last one is command send to STM32.
5.In verification file the one line place a expected result of one command.
