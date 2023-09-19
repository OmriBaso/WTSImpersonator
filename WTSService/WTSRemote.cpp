#define _WINSOCKAPI_
#include <Windows.h>
#include <iostream>
#include <wtsapi32.h>
#pragma comment (lib, "Userenv.lib")
#pragma comment (lib, "Wtsapi32.lib")
#include <winsock2.h>
#include <fstream>
#pragma comment(lib, "ws2_32")


EXTERN_C void ImpersonateExecute(int SessionId, char* Command) {

    HANDLE hToken = 0;
    WTSQueryUserToken(SessionId, &hToken);
    if (!hToken)
        return;


    std::wstring ws(Command, Command + strlen(Command));
    LPCWSTR wCommand = (LPCWSTR)ws.c_str();

    LPCWSTR PATH = wCommand; //change the path accordingly

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD nsid = 1;
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    si.wShowWindow = TRUE;


    if (CreateProcessAsUserW(hToken, PATH, NULL, NULL,
        NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {



        /* Process has been created; work with the process and wait for it to
        terminate. */
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
 

    CloseHandle(hToken);



}