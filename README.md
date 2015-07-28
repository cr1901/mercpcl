# mercpcl
Command Line Mercury (www.micro-nova.com) Programmer

```mercpcl``` is a command-line utility that can be used to program Mercury FPGA development boards without user intervention.
It is essentially a single-thread C translation of the official GUI C# utility that I wrote for use with [Migen/Mibuild](https://github.com/m-labs/migen), released under some sort of FOSS license.

Currently, only programming Mercury dev boards (both the 50000/200000 gate variants) are supported.
Writing arbitrary data to flash and improvements in programming time will follow.

```mercpcl``` requires a C89 compiler that understands line comments (so Microsoft's C Compiler should work), libftdi, and libusb-1.0 (for future compatibility). The utility was developed in a NetBSD environment, and provides a way for using Mercury in environments where .NET does not exist.
