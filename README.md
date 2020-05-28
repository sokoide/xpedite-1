# README.md

## Prereq

* Clone and build Xpedit

```
# install jdk, gradle, gcc/g++ and cmake if not installed
git clone https://github.com/Morgan-Stanley/Xpedite.git
sudo apt install libdw-dev libelf-dev binutils-dev
```

## How to build a test app

* Assuming you have Xpedite repo in ../Xpedite,

```
g++ -o foo foo.cpp -I ../Xpedite/install/include/ -L ../Xpedite/install/lib -pthread -lxpedite-pie -ldl -lrt
```

## How to profile

```
./foo
./scripts/bin/xpedite generate -a /tmp/xpedite-appinfo.txt

vim profileInfo.py # and add homeDir at the end.

./foo
./scripts/bin/xpedite record -p /home/scott/repo/Xpedite/profileInfo.py

# jupyter starts up and you can see transactions
```

