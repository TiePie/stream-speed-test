# stream-speed-test
Commandline utility for testing streaming performance

## Building
- Install libtiepie, see https://www.tiepie.com/libtiepie-sdk
- Compile the application using the GCC compiler by running `make` (or `mingw32-make` on Windows)

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
- `./streamspeedtest -b 8 -c 4 -f 100e6 -l 16777216 -d 300 -r` - Performs a streaming measurement for 5 minutes sampling at 100 MHz @ 8 bit on 4 channels, raw data is passed in 16 MiS blocks to this test application.
