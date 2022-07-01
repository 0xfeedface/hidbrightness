hidbrightness
=============

Command line utility to set the brightness of an Apple Studio Display (2022)
under Linux.

Dependencies
------------
* libhidapi
* C++23 compiler (Makefile is currently hardcoded to Clang)

Building
--------
1. Install dependencies
2. type `make`

Usage
-----
* just running `./hidbrightness` will print the current brightness reading.
* run `./hidbrightness --inc[rease]` to increase brightness to the next step.
* run `./hidbrightness --dec[rease]` to decrease brightness to the previous step.
