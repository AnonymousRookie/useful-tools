#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <Windows.h>
#include <Dbghelp.h>
#include <tchar.h>  
#include <iostream>  
#include <vector>  
#include <sstream>
#include <stdlib.h>

#pragma comment(lib, "Dbghelp.lib")

namespace z
{
// 创建dump文件
void CreateDumpFile(LPCSTR lpstrDumpFilePathName, EXCEPTION_POINTERS* pException)
{
    HANDLE hDumpFile = CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
    return NULL;
}

BOOL PreventSetUnhandledExceptionFilter()
{
    HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
    if (hKernel32 == NULL) {
        return FALSE;
    }
    void* pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
    if (pOrgEntry == NULL) {
        return FALSE;
    }
    unsigned char newJump[100] = { 0 };
    DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
    dwOrgEntryAddr += 5; // jump instruction has 5 byte space
    void* pNewFunc = &DummySetUnhandledExceptionFilter;
    DWORD dwNewEntryAddr = (DWORD) pNewFunc;
    DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
    newJump[0] = 0xE9;  // jump
    memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
    SIZE_T bytesWritten;
    BOOL bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
    return bRet;
}


// 记录CallStack
const int MAX_ADDRESS_LENGTH = 32;
const int MAX_NAME_LENGTH = 1024;

// 崩溃信息
struct CrashInfo
{
    CHAR ErrorCode[MAX_ADDRESS_LENGTH];
    CHAR Address[MAX_ADDRESS_LENGTH];
    CHAR Flags[MAX_ADDRESS_LENGTH];
};

// CallStack信息
struct CallStackInfo
{
    CHAR ModuleName[MAX_NAME_LENGTH];
    CHAR MethodName[MAX_NAME_LENGTH];
    CHAR FileName[MAX_NAME_LENGTH];
    CHAR LineNumber[MAX_NAME_LENGTH];
};

// 安全拷贝字符串函数
void SafeStrCpy(char* szDest, size_t nMaxDestSize, const char* szSrc)
{
    if (nMaxDestSize <= 0) 
        return;
    if (strlen(szSrc) < nMaxDestSize) {
        strcpy_s(szDest, nMaxDestSize, szSrc);
    }
    else {
        strncpy_s(szDest, nMaxDestSize, szSrc, nMaxDestSize);
        szDest[nMaxDestSize-1] = '\0';
    }
}  

// 获取程序崩溃信息
CrashInfo GetCrashInfo(const EXCEPTION_RECORD *pRecord)
{
    CrashInfo crashinfo;
    SafeStrCpy(crashinfo.Address, MAX_ADDRESS_LENGTH, "N/A");
    SafeStrCpy(crashinfo.ErrorCode, MAX_ADDRESS_LENGTH, "N/A");
    SafeStrCpy(crashinfo.Flags, MAX_ADDRESS_LENGTH, "N/A");

    sprintf_s(crashinfo.Address, "%08X", pRecord->ExceptionAddress);
    sprintf_s(crashinfo.ErrorCode, "%08X", pRecord->ExceptionCode);
    sprintf_s(crashinfo.Flags, "%08X", pRecord->ExceptionFlags);

    return crashinfo;
}

// 获取CallStack信息
std::vector<CallStackInfo> GetCallStack(const CONTEXT *pContext)
{
    HANDLE hProcess = GetCurrentProcess();

    SymInitialize(hProcess, NULL, TRUE);

    std::vector<CallStackInfo> arrCallStackInfo;

    CONTEXT c = *pContext;

    STACKFRAME64 sf;
    memset(&sf, 0, sizeof(STACKFRAME64));
    DWORD dwImageType = IMAGE_FILE_MACHINE_I386;

    // 不同的CPU类型，具体信息可查询MSDN
#ifdef _M_IX86
    sf.AddrPC.Offset = c.Eip;
    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrStack.Offset = c.Esp;
    sf.AddrStack.Mode = AddrModeFlat;
    sf.AddrFrame.Offset = c.Ebp;
    sf.AddrFrame.Mode = AddrModeFlat;
#elif _M_X64
    dwImageType = IMAGE_FILE_MACHINE_AMD64;
    sf.AddrPC.Offset = c.Rip;
    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrFrame.Offset = c.Rsp;
    sf.AddrFrame.Mode = AddrModeFlat;
    sf.AddrStack.Offset = c.Rsp;
    sf.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    dwImageType = IMAGE_FILE_MACHINE_IA64;
    sf.AddrPC.Offset = c.StIIP;
    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrFrame.Offset = c.IntSp;
    sf.AddrFrame.Mode = AddrModeFlat;
    sf.AddrBStore.Offset = c.RsBSP;
    sf.AddrBStore.Mode = AddrModeFlat;
    sf.AddrStack.Offset = c.IntSp;
    sf.AddrStack.Mode = AddrModeFlat;
#else
    #error "Platform not supported!"
#endif

    HANDLE hThread = GetCurrentThread();

    while (true)
    {
        // 该函数是实现这个功能的最重要的一个函数
        // 函数的用法以及参数和返回值的具体解释可以查询MSDN
        if (!StackWalk64(dwImageType, hProcess, hThread, &sf, &c, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
            break;

        if (sf.AddrFrame.Offset == 0)
            break;

        CallStackInfo callstackinfo;
        SafeStrCpy(callstackinfo.MethodName, MAX_NAME_LENGTH, "N/A");
        SafeStrCpy(callstackinfo.FileName, MAX_NAME_LENGTH, "N/A");
        SafeStrCpy(callstackinfo.ModuleName, MAX_NAME_LENGTH, "N/A");
        SafeStrCpy(callstackinfo.LineNumber, MAX_NAME_LENGTH, "N/A");

        BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH];
        IMAGEHLP_SYMBOL64 *pSymbol = (IMAGEHLP_SYMBOL64*)symbolBuffer;
        memset(pSymbol, 0, sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH);

        pSymbol->SizeOfStruct = sizeof(symbolBuffer);
        pSymbol->MaxNameLength = MAX_NAME_LENGTH;

        DWORD symDisplacement = 0;

        // 得到函数名
        if (SymGetSymFromAddr64(hProcess, sf.AddrPC.Offset, NULL, pSymbol)) {
            SafeStrCpy(callstackinfo.MethodName, MAX_NAME_LENGTH, pSymbol->Name);
        }

        IMAGEHLP_LINE64 lineInfo;
        memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));

        lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        DWORD dwLineDisplacement;

        // 得到文件名和所在的代码行
        if (SymGetLineFromAddr64(hProcess, sf.AddrPC.Offset, &dwLineDisplacement, &lineInfo)) {
            SafeStrCpy(callstackinfo.FileName, MAX_NAME_LENGTH, lineInfo.FileName);
            sprintf_s(callstackinfo.LineNumber, "%d", lineInfo.LineNumber);
        }

        IMAGEHLP_MODULE64 moduleInfo;
        memset(&moduleInfo, 0, sizeof(IMAGEHLP_MODULE64));

        moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

        // 得到模块名
        if (SymGetModuleInfo64(hProcess, sf.AddrPC.Offset, &moduleInfo)) {
            SafeStrCpy(callstackinfo.ModuleName, MAX_NAME_LENGTH, moduleInfo.ModuleName);
        }

        arrCallStackInfo.push_back(callstackinfo);
    }

    SymCleanup(hProcess);

    return arrCallStackInfo;
}


// 处理Unhandled Exception的回调函数
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{	
    // 确保有足够的栈空间
#ifdef _M_IX86
    if (pException->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
    {
        static char TempStack[1024 * 128];
        __asm mov eax,offset TempStack[1024 * 128];
        __asm mov esp,eax;
    }
#endif	

    CrashInfo crashinfo = GetCrashInfo(pException->ExceptionRecord);

    std::ostringstream oss("");
    // 输出Crash信息
    oss << "ErrorCode: " << crashinfo.ErrorCode << "\n";
    oss << "Address: " << crashinfo.Address << "\n";
    oss << "Flags: " << crashinfo.Flags << "\n";

    std::vector<CallStackInfo> arrCallStackInfo = GetCallStack(pException->ContextRecord);

    // 输出CallStack
    oss << "CallStack: " << "\n";
    std::vector<CallStackInfo>::iterator iter = arrCallStackInfo.begin();
    for (; iter != arrCallStackInfo.end(); ++iter) {
        CallStackInfo callstackinfo = (*iter);
        oss << callstackinfo.MethodName << "() : [" << callstackinfo.ModuleName << "] (File: " << callstackinfo.FileName << " @Line " << callstackinfo.LineNumber << ")" << "\n";
    }

    // 记录到log中
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);
    TCHAR szCallStackFile[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, szCallStackFile, MAX_PATH);
    TCHAR* pFind = _tcsrchr(szCallStackFile, '\\');
    if (pFind)
    {
        *(pFind + 1) = 0;

        TCHAR szFileFormat[MAX_PATH] = { 0 };
        sprintf_s(szFileFormat, sizeof(szFileFormat), "%d_%d_%d_%d_%d_%d_callstack.log", 
            localTime.wYear, localTime.wMonth, localTime.wDay, 
            localTime.wHour, localTime.wMinute, localTime.wSecond);

        _tcscat(szCallStackFile, szFileFormat);

        FILE* fp = fopen(szCallStackFile, "w+");
        if (fp != NULL) {
            fputs(oss.str().c_str(), fp);
            fclose(fp);
        }
    }

    // 生成dump文件
    TCHAR szMbsFile[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, szMbsFile, MAX_PATH);
    pFind = _tcsrchr(szMbsFile, '\\');
    if (pFind)
    {
        *(pFind + 1) = 0;

        TCHAR szFileFormat[MAX_PATH] = { 0 };
        sprintf_s(szFileFormat, sizeof(szFileFormat), "%d_%d_%d_%d_%d_%d_crash.dmp", 
            localTime.wYear, localTime.wMonth, localTime.wDay, 
            localTime.wHour, localTime.wMinute, localTime.wSecond);

        _tcscat(szMbsFile, szFileFormat);   

        CreateDumpFile(szMbsFile, pException);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void RunCrashHandler()
{
    // 设置处理Unhandled Exception的回调函数
    // 记录堆栈信息并生成dump文件
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
    PreventSetUnhandledExceptionFilter();
}

}  // namespace z


#endif  // CRASH_HANDLER_H