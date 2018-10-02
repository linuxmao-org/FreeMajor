[![Build Status](https://semaphoreci.com/api/v1/jpcima/gmajctl-2/branches/master/badge.svg)](https://semaphoreci.com/jpcima/gmajctl-2)

[Automatic development build for Windows](http://jpcima.sdf1.org/software/development/GMajCtl/gmajctl-dev-win32.zip)

# Première compilation / First compilation

```
git clone --recursive https://github.com/jpcima/gmajctl.git
mkdir gmajctl/build
cd gmajctl/build
cmake ..
cmake --build .
cd ..
./build/gmajctl
```
# Mise à jour et compilation suivantes / build after some modifications

```
cd gmajctl
git pull
cmake --build build
./build/gmajctl
```
