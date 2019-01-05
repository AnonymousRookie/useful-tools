# Windows平台下程序crash时记录堆栈信息并生成dump文件

- 2019_1_5_20_1_23_crash.dmp
- 2019_1_5_20_1_23_callstack.log
```
ErrorCode: C0000094
Address: 00353785
Flags: 00000000
CallStack: 
main() : [hh] (File: c:\visual studio 2010\projects\test_crash_handler\test.cpp @Line 11)
__tmainCRTStartup() : [hh] (File: f:\dd\vctools\crt_bld\self_x86\crt\src\crtexe.c @Line 555)
mainCRTStartup() : [hh] (File: f:\dd\vctools\crt_bld\self_x86\crt\src\crtexe.c @Line 371)
BaseThreadInitThunk() : [kernel32] (File: N/A @Line N/A)
RtlInitializeExceptionChain() : [ntdll] (File: N/A @Line N/A)
RtlInitializeExceptionChain() : [ntdll] (File: N/A @Line N/A)
```