# stream-speed-test
Commandline utility for testing streaming performance

## Building
- Install libtiepie, see http://www.tiepie.com/en/SDK/LibTiePie/Linux
- Compile the application using by running `make`

## Command line options
- `-b <resolution in bits>` - Resolution for measurement. (default: highest possible)
- `-c <number of channels>` - Number of active channels for measurement. (default: all)
- `-f <sample frequency` - Sample frequency in Hz. (default: 10kHz)
- `-l <record length>` - Record length in Samples. (default: 5000)
- `-d <duration>` - Duration of the measurement in seconds. (default: 60)
- `-s <serial number>` - Open instrument with specified serial number. (default: auto detect)
- `-r` - When specified the raw data to floating point data conversion is disabled.

## Example
- `./streamspeedtest -b 14 -c 1 -f 10000000 -l 5000000 -d 30 -r` - Performs a streaming measurement for 30 seconds sampling at 10 MHz @ 14 bit, raw data is passed in 5 MS blocks to this test application.
