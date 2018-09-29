# Première compilation / First compilation

```
git clone --recursive https://github.com/jpcima/gmajctl.git
mkdir gmajctl/build
cd gmajctl/build
cmake ..
cmake --build .
./build/gmajctl
```
# Mise à jour et compilation suivantes / build after some modifications

```
cd gmajctl
git pull
cmake --build build
./build/gmajctl
```
