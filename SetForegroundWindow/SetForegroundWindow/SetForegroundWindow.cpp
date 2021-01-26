// SetForegroundWindow.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdint.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <winuser.h>

// ���ο���̨Ӧ�ó���Ĵ���
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

struct EnumWindowsArg
{
    HWND hwndWindow;   // ���ھ��
    DWORD dwProcessID; // ����ID
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowsArg *pArg = (EnumWindowsArg*)lParam;
    DWORD dwProcessID = 0;
    // ͨ�����ھ��ȡ�ý���ID
    ::GetWindowThreadProcessId(hwnd, &dwProcessID);
    if (dwProcessID == pArg->dwProcessID)
    {
        pArg->hwndWindow = hwnd;
        return FALSE;
    }
    return TRUE;
}

// ͨ������ID��ȡ���ھ��
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
    // �÷�: SetForegroundWindow.exe ��Ҫ��ǰ��ʾ�ĳ���.exe
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
