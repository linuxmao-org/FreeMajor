[![Build Status](https://semaphoreci.com/api/v1/jpcima/gmajctl-2/branches/master/badge.svg)](https://semaphoreci.com/jpcima/gmajctl-2)

[Automatic development build for Windows](http://jpcima.sdf1.org/software/development/GMajCtl/gmajctl-dev-win32.zip)

### Dependencies
gmajctl is mostly tested on [LibraZiK-2](http://librazik.tuxfamily.org/), a Debian Stretch based OS.
The build dependencies on this OS are:
```
cmake
build-essential
libasound2-dev
libjack-jackd2-dev
libfltk1.3-dev
gettext
```
Note that you might need to install `git` as well to fetch the sources.

### First compilation

```
git clone --recursive https://github.com/linuxmao-org/FreeMajor.git
mkdir gmajctl/build
cd gmajctl/build
cmake ..
cmake --build .
cd ..
./build/gmajctl
```
### Build after some modifications

```
cd gmajctl
git pull
cmake --build build
./build/gmajctl
```
