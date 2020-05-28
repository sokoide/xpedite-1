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
215   ip = "0.0.0.0"
```

* On Windows, make %USERPROFILE%\\.wslconfig

```
localhostForwarding=True
```

* After running the report, access http://localhost:8888/...(copy from the WSL2 output) from Windows's browser


## What's injected

* When you compile foo.cpp with the options I described above, the following Xpedite code was injected at 848a, 84a0 and the callers in `_Z4lifei`.
* When you compile it with -O2, it injects a few nops instead.
* Those are replaced with the actual instumentation code at runtime when you enable it.

* in foo.cpp:

```cpp
void life(int timeToLive_) {
  for(unsigned i=0; i<timeToLive_; ++i) {
    XPEDITE_TXN_SCOPE(Life);
    eat();

    XPEDITE_PROBE(SleepBegin);
    sleep();

    XPEDITE_PROBE(CodeBegin);
    code();
  }
}
```

* objdump -M Intel -d foo

```asm
## If you compile with -O2
0000000000008690 <_Z4lifei>:
    8690:	f3 0f 1e fa          	endbr64
    8694:	85 ff                	test   edi,edi
    8696:	74 58                	je     86f0 <_Z4lifei+0x60>
    8698:	55                   	push   rbp
    8699:	89 fd                	mov    ebp,edi
    869b:	53                   	push   rbx
    869c:	31 db                	xor    ebx,ebx
    869e:	48 83 ec 08          	sub    rsp,0x8
    86a2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    86a8:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    86ad:	e8 2e fe ff ff       	call   84e0 <_Z3eatv>
    86b2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    86b8:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    86bd:	e8 ae fe ff ff       	call   8570 <_Z5sleepv>
    86c2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    86c8:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    86cd:	e8 2e ff ff ff       	call   8600 <_Z4codev>
    86d2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    86d8:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    86dd:	83 c3 01             	add    ebx,0x1
    86e0:	39 dd                	cmp    ebp,ebx
    86e2:	75 c4                	jne    86a8 <_Z4lifei+0x18>
    86e4:	48 83 c4 08          	add    rsp,0x8
    86e8:	5b                   	pop    rbx
    86e9:	5d                   	pop    rbp
    86ea:	c3                   	ret
    86eb:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    86f0:	c3                   	ret
    86f1:	f3 0f 1e fa          	endbr64
    86f5:	e9 e6 f2 ff ff       	jmp    79e0 <_Z4lifei.cold.5>
    86fa:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

## If you compile w/o -Ox
000000000000848a <_ZZ4lifeiEN18XpediteGuardLife12C1Ev>:
    848a:	55                   	push   rbp
    848b:	48 89 e5             	mov    rbp,rsp
    848e:	48 89 7d f8          	mov    QWORD PTR [rbp-0x8],rdi
    8492:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    8498:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    849d:	90                   	nop
    849e:	5d                   	pop    rbp
    849f:	c3                   	ret

00000000000084a0 <_ZZ4lifeiEN18XpediteGuardLife12D1Ev>:
    84a0:	55                   	push   rbp
    84a1:	48 89 e5             	mov    rbp,rsp
    84a4:	48 89 7d f8          	mov    QWORD PTR [rbp-0x8],rdi
    84a8:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    84ad:	90                   	nop
    84ae:	5d                   	pop    rbp
    84af:	c3                   	ret    0000000

0000084b0 <_Z4lifei>:
    84b0:	f3 0f 1e fa          	endbr64
    84b4:	55                   	push   rbp
    84b5:	48 89 e5             	mov    rbp,rsp
    84b8:	53                   	push   rbx
    84b9:	48 83 ec 28          	sub    rsp,0x28
    84bd:	89 7d dc             	mov    DWORD PTR [rbp-0x24],edi
    84c0:	64 48 8b 04 25 28 00 	mov    rax,QWORD PTR fs:0x28
    84c7:	00 00
    84c9:	48 89 45 e8          	mov    QWORD PTR [rbp-0x18],rax
    84cd:	31 c0                	xor    eax,eax
    84cf:	c7 45 e4 00 00 00 00 	mov    DWORD PTR [rbp-0x1c],0x0
    84d6:	8b 45 dc             	mov    eax,DWORD PTR [rbp-0x24]
    84d9:	39 45 e4             	cmp    DWORD PTR [rbp-0x1c],eax
    84dc:	73 5c                	jae    853a <_Z4lifei+0x8a>
    84de:	48 8d 45 e3          	lea    rax,[rbp-0x1d]
    84e2:	48 89 c7             	mov    rdi,rax
    84e5:	e8 a0 ff ff ff       	call   848a <_ZZ4lifeiEN18XpediteGuardLife12C1Ev>
    84ea:	e8 01 ff ff ff       	call   83f0 <_Z3eatv>
    84ef:	90                   	nop
    84f0:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    84f5:	e8 29 ff ff ff       	call   8423 <_Z5sleepv>
    84fa:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    8500:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    8505:	e8 4c ff ff ff       	call   8456 <_Z4codev>
    850a:	48 8d 45 e3          	lea    rax,[rbp-0x1d]
    850e:	48 89 c7             	mov    rdi,rax
    8511:	e8 8a ff ff ff       	call   84a0 <_ZZ4lifeiEN18XpediteGuardLife12D1Ev>
    8516:	83 45 e4 01          	add    DWORD PTR [rbp-0x1c],0x1
    851a:	eb ba                	jmp    84d6 <_Z4lifei+0x26>
    851c:	f3 0f 1e fa          	endbr64
    8520:	48 89 c3             	mov    rbx,rax
    8523:	48 8d 45 e3          	lea    rax,[rbp-0x1d]
    8527:	48 89 c7             	mov    rdi,rax
    852a:	e8 71 ff ff ff       	call   84a0 <_ZZ4lifeiEN18XpediteGuardLife12D1Ev>
    852f:	48 89 d8             	mov    rax,rbx
    8532:	48 89 c7             	mov    rdi,rax
    8535:	e8 86 f3 ff ff       	call   78c0 <_Unwind_Resume@plt>
    853a:	90                   	nop
    853b:	48 8b 45 e8          	mov    rax,QWORD PTR [rbp-0x18]
    853f:	64 48 33 04 25 28 00 	xor    rax,QWORD PTR fs:0x28
    8546:	00 00
    8548:	74 05                	je     854f <_Z4lifei+0x9f>
    854a:	e8 01 f0 ff ff       	call   7550 <__stack_chk_fail@plt>
    854f:	48 83 c4 28          	add    rsp,0x28
    8553:	5b                   	pop    rbx
    8554:	5d                   	pop    rbp
    8555:	c3                   	ret
```
