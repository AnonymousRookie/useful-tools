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
// ����dump�ļ�
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


// ��¼CallStack
const int MAX_ADDRESS_LENGTH = 32;
const int MAX_NAME_LENGTH = 1024;

// ������Ϣ
struct CrashInfo
{
    CHAR ErrorCode[MAX_ADDRESS_LENGTH];
    CHAR Address[MAX_ADDRESS_LENGTH];
    CHAR Flags[MAX_ADDRESS_LENGTH];
};

// CallStack��Ϣ
struct CallStackInfo
{
    CHAR ModuleName[MAX_NAME_LENGTH];
    CHAR MethodName[MAX_NAME_LENGTH];
    CHAR FileName[MAX_NAME_LENGTH];
    CHAR LineNumber[MAX_NAME_LENGTH];
};

// ��ȫ�����ַ�������
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

// ��ȡ���������Ϣ
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

// ��ȡCallStack��Ϣ
std::vector<CallStackInfo> GetCallStack(const CONTEXT *pContext)
{
    HANDLE hProcess = GetCurrentProcess();

    SymInitialize(hProcess, NULL, TRUE);

    std::vector<CallStackInfo> arrCallStackInfo;

    CONTEXT c = *pContext;

    STACKFRAME64 sf;
    memset(&sf, 0, sizeof(STACKFRAME64));
    DWORD dwImageType = IMAGE_FILE_MACHINE_I386;

    // ��ͬ��CPU���ͣ�������Ϣ�ɲ�ѯMSDN
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
        // �ú�����ʵ��������ܵ�����Ҫ��һ������
        // �������÷��Լ������ͷ���ֵ�ľ�����Ϳ��Բ�ѯMSDN
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

        // �õ�������
        if (SymGetSymFromAddr64(hProcess, sf.AddrPC.Offset, NULL, pSymbol)) {
            SafeStrCpy(callstackinfo.MethodName, MAX_NAME_LENGTH, pSymbol->Name);
        }

        IMAGEHLP_LINE64 lineInfo;
        memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));

        lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        DWORD dwLineDisplacement;

        // �õ��ļ��������ڵĴ�����
        if (SymGetLineFromAddr64(hProcess, sf.AddrPC.Offset, &dwLineDisplacement, &lineInfo)) {
            SafeStrCpy(callstackinfo.FileName, MAX_NAME_LENGTH, lineInfo.FileName);
            sprintf_s(callstackinfo.LineNumber, "%d", lineInfo.LineNumber);
        }

        IMAGEHLP_MODULE64 moduleInfo;
        memset(&moduleInfo, 0, sizeof(IMAGEHLP_MODULE64));

        moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

        // �õ�ģ����
        if (SymGetModuleInfo64(hProcess, sf.AddrPC.Offset, &moduleInfo)) {
            SafeStrCpy(callstackinfo.ModuleName, MAX_NAME_LENGTH, moduleInfo.ModuleName);
        }

        arrCallStackInfo.push_back(callstackinfo);
    }

    SymCleanup(hProcess);

    return arrCallStackInfo;
}


// ����Unhandled Exception�Ļص�����
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{	
    // ȷ�����㹻��ջ�ռ�
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
    // ���Crash��Ϣ
    oss << "ErrorCode: " << crashinfo.ErrorCode << "\n";
    oss << "Address: " << crashinfo.Address << "\n";
    oss << "Flags: " << crashinfo.Flags << "\n";

    std::vector<CallStackInfo> arrCallStackInfo = GetCallStack(pException->ContextRecord);

    // ���CallStack
    oss << "CallStack: " << "\n";
    std::vector<CallStackInfo>::iterator iter = arrCallStackInfo.begin();
    for (; iter != arrCallStackInfo.end(); ++iter) {
        CallStackInfo callstackinfo = (*iter);
        oss << callstackinfo.MethodName << "() : [" << callstackinfo.ModuleName << "] (File: " << callstackinfo.FileName << " @Line " << callstackinfo.LineNumber << ")" << "\n";
    }

    // ��¼��log��
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

    // ����dump�ļ�
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
    // ���ô���Unhandled Exception�Ļص�����
    // ��¼��ջ��Ϣ������dump�ļ�
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
    PreventSetUnhandledExceptionFilter();
}

}  // namespace z


#endif  // CRASH_HANDLER_H