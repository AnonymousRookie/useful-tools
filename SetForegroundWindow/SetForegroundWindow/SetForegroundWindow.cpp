// SetForegroundWindow.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdint.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <winuser.h>

// 屏蔽控制台应用程序的窗口
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

struct EnumWindowsArg
{
    HWND hwndWindow;   // 窗口句柄
    DWORD dwProcessID; // 进程ID
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowsArg *pArg = (EnumWindowsArg*)lParam;
    DWORD dwProcessID = 0;
    // 通过窗口句柄取得进程ID
    ::GetWindowThreadProcessId(hwnd, &dwProcessID);
    if (dwProcessID == pArg->dwProcessID)
    {
        pArg->hwndWindow = hwnd;
        return FALSE;
    }
    return TRUE;
}

// 通过进程ID获取窗口句柄
HWND GetWindowHwndByPID(DWORD dwProcessID)
{
    HWND hwndRet = NULL;
    EnumWindowsArg ewa;
    ewa.dwProcessID = dwProcessID;
    ewa.hwndWindow = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&ewa);
    if (ewa.hwndWindow)
    {
        hwndRet = ewa.hwndWindow;
    }
    return hwndRet;
}

DWORD GetProcessIDByName(const char* pName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
        return NULL;
    }
    PROCESSENTRY32 pe = { sizeof(pe) };
    for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe)) 
    {
        if (strcmp(pe.szExeFile, pName) == 0) {
            CloseHandle(hSnapshot);
            return pe.th32ProcessID;
        }
    }
    CloseHandle(hSnapshot);
    return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
    // 用法: SetForegroundWindow.exe 需要最前显示的程序.exe
    if (argc != 2)
    {
        return -1;
    }

    DWORD processId = GetProcessIDByName(argv[1]);
    if (processId)
    {
        HWND hWnd = GetWindowHwndByPID(processId);
        SetForegroundWindow(hWnd);
    }
    return 0;
}
