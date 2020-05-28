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
* When you compile it with -O2, it injects a few nops instead at 8712, 8718 or etc.
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
00000000000086f0 <_Z4lifei>:
    86f0:	f3 0f 1e fa          	endbr64
    86f4:	85 ff                	test   edi,edi
    86f6:	74 58                	je     8750 <_Z4lifei+0x60>
    86f8:	55                   	push   rbp
    86f9:	89 fd                	mov    ebp,edi
    86fb:	53                   	push   rbx
    86fc:	31 db                	xor    ebx,ebx
    86fe:	48 83 ec 08          	sub    rsp,0x8
    8702:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    8708:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    870d:	e8 2e fe ff ff       	call   8540 <_Z3eatv>
    8712:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    8718:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    871d:	e8 ae fe ff ff       	call   85d0 <_Z5sleepv>
    8722:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    8728:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    872d:	e8 2e ff ff ff       	call   8660 <_Z4codev>
    8732:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
    8738:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    873d:	83 c3 01             	add    ebx,0x1
    8740:	39 dd                	cmp    ebp,ebx
    8742:	75 c4                	jne    8708 <_Z4lifei+0x18>
    8744:	48 83 c4 08          	add    rsp,0x8
    8748:	5b                   	pop    rbx
    8749:	5d                   	pop    rbp
    874a:	c3                   	ret
    874b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    8750:	c3                   	ret
    8751:	f3 0f 1e fa          	endbr64
    8755:	e9 86 f2 ff ff       	jmp    79e0 <_Z4lifei.cold.9>
    875a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

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

* When you take a look at the binary after the probes are enabled with gdb,

```asm
(gdb) disass life
Dump of assembler code for function _Z4lifei:
   0x000055e8ce6426f0 <+0>:     endbr64
   0x000055e8ce6426f4 <+4>:     test   edi,edi
   0x000055e8ce6426f6 <+6>:     je     0x55e8ce642750 <_Z4lifei+96>
   0x000055e8ce6426f8 <+8>:     push   rbp
   0x000055e8ce6426f9 <+9>:     mov    ebp,edi
   0x000055e8ce6426fb <+11>:    push   rbx
   0x000055e8ce6426fc <+12>:    xor    ebx,ebx
   0x000055e8ce6426fe <+14>:    sub    rsp,0x8
   0x000055e8ce642702 <+18>:    nop    WORD PTR [rax+rax*1+0x0]
   0x000055e8ce642708 <+24>:    jmp    0x55e8ce677fc8
   0x000055e8ce64270d <+29>:    call   0x55e8ce642540 <_Z3eatv>
   0x000055e8ce642712 <+34>:    nop    WORD PTR [rax+rax*1+0x0]
   0x000055e8ce642718 <+40>:    jmp    0x55e8ce678034
   0x000055e8ce64271d <+45>:    call   0x55e8ce6425d0 <_Z5sleepv>
   0x000055e8ce642722 <+50>:    nop    WORD PTR [rax+rax*1+0x0]
   0x000055e8ce642728 <+56>:    jmp    0x55e8ce6780a4
   0x000055e8ce64272d <+61>:    call   0x55e8ce642660 <_Z4codev>
   0x000055e8ce642732 <+66>:    nop    WORD PTR [rax+rax*1+0x0]
   0x000055e8ce642738 <+72>:    jmp    0x55e8ce678114
   0x000055e8ce64273d <+77>:    add    ebx,0x1
   0x000055e8ce642740 <+80>:    cmp    ebp,ebx
   0x000055e8ce642742 <+82>:    jne    0x55e8ce642708 <_Z4lifei+24>
   0x000055e8ce642744 <+84>:    add    rsp,0x8
   0x000055e8ce642748 <+88>:    pop    rbx
   0x000055e8ce642749 <+89>:    pop    rbp
   0x000055e8ce64274a <+90>:    ret
   0x000055e8ce64274b <+91>:    nop    DWORD PTR [rax+rax*1+0x0]
   0x000055e8ce642750 <+96>:    ret
   0x000055e8ce642751 <+97>:    endbr64
   0x000055e8ce642755 <+101>:   jmp    0x55e8ce6419e0 <_Z4lifei.cold.9>
End of assembler dump.

(gdb) disass 0x55e8ce660410
Dump of assembler code for function xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite):
   0x000055e8ce660410 <+0>:     test   rdi,rdi
   0x000055e8ce660413 <+3>:     je     0x55e8ce6604b0 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+160>
   0x000055e8ce660419 <+9>:     push   r12
   0x000055e8ce66041b <+11>:    push   rbp
   0x000055e8ce66041c <+12>:    push   rbx
   0x000055e8ce66041d <+13>:    mov    rbx,rdi
   0x000055e8ce660420 <+16>:    call   0x55e8ce65e800 <xpedite::probes::Probe::isValid(xpedite::probes::Instructions volatile*, xpedite::probes::Instructions volatile*) const>
   0x000055e8ce660425 <+21>:    mov    rbp,QWORD PTR [rip+0x252d4]        # 0x55e8ce685700 <_ZN7xpedite6probes6Config9_instanceE>
   0x000055e8ce66042c <+28>:    mov    r12d,eax
   0x000055e8ce66042f <+31>:    test   rbp,rbp
   0x000055e8ce660432 <+34>:    je     0x55e8ce660490 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+128>
   0x000055e8ce660434 <+36>:    cmp    BYTE PTR [rbp+0x0],0x0
   0x000055e8ce660438 <+40>:    jne    0x55e8ce6604d0 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+192>
   0x000055e8ce66043e <+46>:    test   r12b,r12b
   0x000055e8ce660441 <+49>:    je     0x55e8ce660485 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+117>
   0x000055e8ce660443 <+51>:    cmp    QWORD PTR [rbx+0x20],0x0
   0x000055e8ce660448 <+56>:    jne    0x55e8ce660560 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+336>
   0x000055e8ce66044e <+62>:    mov    rax,QWORD PTR [rip+0x252bb]        # 0x55e8ce685710 <_ZN7xpedite6probes9ProbeList9_instanceE>
   0x000055e8ce660455 <+69>:    test   rax,rax
   0x000055e8ce660458 <+72>:    je     0x55e8ce660520 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+272>
   0x000055e8ce66045e <+78>:    mov    ecx,DWORD PTR [rax+0x8]
   0x000055e8ce660461 <+81>:    mov    rdx,QWORD PTR [rax]
   0x000055e8ce660464 <+84>:    lea    esi,[rcx+0x1]
   0x000055e8ce660467 <+87>:    test   rdx,rdx
   0x000055e8ce66046a <+90>:    mov    DWORD PTR [rax+0x8],esi
   0x000055e8ce66046d <+93>:    mov    DWORD PTR [rbx+0x50],ecx
   0x000055e8ce660470 <+96>:    mov    QWORD PTR [rbx+0x28],0x0
   0x000055e8ce660478 <+104>:   mov    QWORD PTR [rbx+0x20],rdx
   0x000055e8ce66047c <+108>:   je     0x55e8ce660482 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+114>
   0x000055e8ce66047e <+110>:   mov    QWORD PTR [rdx+0x28],rbx
   0x000055e8ce660482 <+114>:   mov    QWORD PTR [rax],rbx
   0x000055e8ce660485 <+117>:   pop    rbx
   0x000055e8ce660486 <+118>:   pop    rbp
   0x000055e8ce660487 <+119>:   pop    r12
   0x000055e8ce660489 <+121>:   ret
   0x000055e8ce66048a <+122>:   nop    WORD PTR [rax+rax*1+0x0]
   0x000055e8ce660490 <+128>:   mov    edi,0x1
   0x000055e8ce660495 <+133>:   call   0x55e8ce6414f0 <_Znwm@plt>
   0x000055e8ce66049a <+138>:   mov    rbp,rax
   0x000055e8ce66049d <+141>:   mov    rdi,rax
   0x000055e8ce6604a0 <+144>:   call   0x55e8ce65e750 <xpedite::probes::Config::Config()>
   0x000055e8ce6604a5 <+149>:   mov    QWORD PTR [rip+0x25254],rbp        # 0x55e8ce685700 <_ZN7xpedite6probes6Config9_instanceE>
   0x000055e8ce6604ac <+156>:   jmp    0x55e8ce660434 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+36>
   0x000055e8ce6604ae <+158>:   xchg   ax,ax
   0x000055e8ce6604b0 <+160>:   mov    rcx,QWORD PTR [rip+0x25069]        # 0x55e8ce685520 <stderr@@GLIBC_2.2.5>
   0x000055e8ce6604b7 <+167>:   lea    rdi,[rip+0x1a8aa]        # 0x55e8ce67ad68
   0x000055e8ce6604be <+174>:   mov    edx,0x34
   0x000055e8ce6604c3 <+179>:   mov    esi,0x1
   0x000055e8ce6604c8 <+184>:   jmp    0x55e8ce641990 <fwrite@plt>
   0x000055e8ce6604cd <+189>:   nop    DWORD PTR [rax]
   0x000055e8ce6604d0 <+192>:   cmp    QWORD PTR [rbx],0x0
   0x000055e8ce6604d4 <+196>:   je     0x55e8ce660590 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+384>
   0x000055e8ce6604da <+202>:   test   r12b,r12b
   0x000055e8ce6604dd <+205>:   lea    rax,[rip+0x1a840]        # 0x55e8ce67ad24
   0x000055e8ce6604e4 <+212>:   je     0x55e8ce660590 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+384>
   0x000055e8ce6604ea <+218>:   sub    rsp,0x8
   0x000055e8ce6604ee <+222>:   mov    rcx,QWORD PTR [rbx+0x30]
   0x000055e8ce6604f2 <+226>:   mov    rdi,QWORD PTR [rip+0x25027]        # 0x55e8ce685520 <stderr@@GLIBC_2.2.5>
   0x000055e8ce6604f9 <+233>:   push   rax
   0x000055e8ce6604fa <+234>:   mov    r9d,DWORD PTR [rbx+0x48]
   0x000055e8ce6604fe <+238>:   lea    rdx,[rip+0x1a89b]        # 0x55e8ce67ada0
   0x000055e8ce660505 <+245>:   mov    r8,QWORD PTR [rbx+0x38]
   0x000055e8ce660509 <+249>:   mov    esi,0x1
   0x000055e8ce66050e <+254>:   xor    eax,eax
   0x000055e8ce660510 <+256>:   call   0x55e8ce641820 <__fprintf_chk@plt>
   0x000055e8ce660515 <+261>:   pop    rax
   0x000055e8ce660516 <+262>:   pop    rdx
   0x000055e8ce660517 <+263>:   jmp    0x55e8ce66043e <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+46>
   0x000055e8ce66051c <+268>:   nop    DWORD PTR [rax+0x0]
   0x000055e8ce660520 <+272>:   mov    edi,0x10
   0x000055e8ce660525 <+277>:   call   0x55e8ce6414f0 <_Znwm@plt>
   0x000055e8ce66052a <+282>:   mov    DWORD PTR [rbx+0x50],0x0
   0x000055e8ce660531 <+289>:   mov    QWORD PTR [rax],0x0
   0x000055e8ce660538 <+296>:   mov    QWORD PTR [rip+0x251d1],rax        # 0x55e8ce685710 <_ZN7xpedite6probes9ProbeList9_instanceE>
   0x000055e8ce66053f <+303>:   mov    DWORD PTR [rax+0x8],0x1
   0x000055e8ce660546 <+310>:   mov    QWORD PTR [rbx+0x28],0x0
   0x000055e8ce66054e <+318>:   mov    QWORD PTR [rbx+0x20],0x0
   0x000055e8ce660556 <+326>:   jmp    0x55e8ce660482 <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+114>
   0x000055e8ce66055b <+331>:   nop    DWORD PTR [rax+rax*1+0x0]
   0x000055e8ce660560 <+336>:   mov    rcx,QWORD PTR [rbx+0x30]
   0x000055e8ce660564 <+340>:   mov    r9d,DWORD PTR [rbx+0x48]
   0x000055e8ce660568 <+344>:   lea    rdx,[rip+0x1a861]        # 0x55e8ce67add0
   0x000055e8ce66056f <+351>:   mov    r8,QWORD PTR [rbx+0x38]
   0x000055e8ce660573 <+355>:   mov    rdi,QWORD PTR [rip+0x24fa6]        # 0x55e8ce685520 <stderr@@GLIBC_2.2.5>
   0x000055e8ce66057a <+362>:   mov    esi,0x1
   0x000055e8ce66057f <+367>:   pop    rbx
   0x000055e8ce660580 <+368>:   pop    rbp
   0x000055e8ce660581 <+369>:   pop    r12
   0x000055e8ce660583 <+371>:   xor    eax,eax
   0x000055e8ce660585 <+373>:   jmp    0x55e8ce641820 <__fprintf_chk@plt>
   0x000055e8ce66058a <+378>:   nop    WORD PTR [rax+rax*1+0x0]
   0x000055e8ce660590 <+384>:   lea    rax,[rip+0x1a78b]        # 0x55e8ce67ad22
   0x000055e8ce660597 <+391>:   jmp    0x55e8ce6604ea <xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite)+218>
```
