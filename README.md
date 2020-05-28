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

## To access Jupyter nootbook in WSL2 from Windows host

* edit scripts/lib/xpedite/jupyter/driver.py

```py
195 def launchJupyter(homeDir):
196   """
197   Method to set env vars for overriding jup config, adding
198    python path and extensions, and finally launching jupyter
199
200   """
201   from xpedite.jupyter         import SHELL_PREFIX, DATA_DIR
202   from xpedite.dependencies    import binPath
203   LOGGER.info('')
204   pyPath = os.path.dirname(binPath('python')) + os.pathsep + os.environ['PATH']
205   initPath = os.path.dirname(__file__)
206   jupyterEnv = os.environ
207   jupyterEnv[Context.dataPathKey] = os.path.join(os.path.abspath(homeDir), DATA_DIR)
208   jupyterEnv['JUPYTER_PATH'] = os.path.join(initPath, '../../../jupyter/extensions/')
209   jupyterEnv['JUPYTER_CONFIG_DIR'] = os.path.join(initPath, '../../../jupyter/config/')
210   jupyterEnv['XPEDITE_PATH'] = os.path.abspath(os.path.join(initPath, '../../'))
211   jupyterEnv['PATH'] = pyPath
212   jupyterEnv['HOME'] = tempfile.mkdtemp(prefix=SHELL_PREFIX, dir='/tmp')
213   ### change here
214   #ip = socket.gethostbyname(socket.gethostname())
215   ip = "0.0.0.0"i195 def launchJupyter(homeDir):
```

* On Windows, make %USERPROFILE%\\.wslconfig

```
localhostForwarding=True
```

* After running the report, access http://localhost:8888/...(copy from the WSL2 output) from Windows's browser
