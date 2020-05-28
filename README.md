# README.md

## Prereq

* Clone and build Xpedit

```
# install jdk, gradle, gcc/g++ and cmake if not installed
git clone https://github.com/Morgan-Stanley/Xpedite.git
sudo apt install libdw-dev libelf-dev binutils-dev

# build Xpedite as instructed

# I didn't run install.sh, but installed specified pip modules in my pyenv-virtualenv for Py 2.7
```

## Tested Environment

### Successful

* Ubuntu 18.04 VM, g++7.5 w/o Xpedite change
* Ubuntu 20.04 on WSL2, g++8.4 or g++7.5 with Xpedite code change because binutils changed the interface  -> Change the followings in vivify/lib/vivify/util/CallResolver.C

```cpp
   66 void findAddrInSection(bfd* bfd_, asection* section_, void* ctxt_) noexcept {
   67     auto& l_ctxt{*static_cast<CallResolverCtxt*>(ctxt_)};
   68
   69     if (l_ctxt._stop) {
   70         return;
   71     }
>  72     // ### change here
   73     // if (0 == (bfd_get_section_flags(bfd_, section_) & SEC_ALLOC))
   74     if (0 == (bfd_section_flags(section_) &
   75               SEC_ALLOC)) { // if not allocated, it is not a debug info section
   76         return;
   77     }
>  78     // ### change here
   79     // const auto l_vma{bfd_get_section_vma(bfd_, section_)};
   80     const auto l_vma{bfd_section_vma(section_)};
   81     if (l_ctxt._pc < l_vma ||
>  82         // ### change here
   83         // l_ctxt._pc >= l_vma + bfd_get_section_size(section_)) {
   84         l_ctxt._pc >= l_vma + bfd_section_size(section_)) {
   85         return;
   86     }
```


### Failed

* Ubuntu 20.04 on WSL2, g++10/9 fails with `error: a reinterpret_cast is not a constant expression` in Xpedite build.


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

