# How to build a faster perf binary

https://michcioperz.com/wiki/slow-perf-script/



```
sudo apt-get install binutils-dev 
sudo apt source linux-tools-common
cd linux-5.15.0/tools/perf/
make prefix=$HOME/.local install-bin
```
