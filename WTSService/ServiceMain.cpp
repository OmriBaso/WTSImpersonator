#include "ServiceMain.h"

#include <iostream>
#include <fstream>
#include <Windows.h>

#include "WTSRemote.h"

// Function to handle service control requests
VOID WINAPI ServiceCtrlHandler(DWORD);


// Function to start the service
VOID WINAPI ServiceMain(DWORD, LPTSTR[]);

// Global variables
SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle;
DWORD SessionId = {};
char* CommandLine = {};

int main(int argc, char* argv[])
{

    SessionId = atoi(argv[1]);
    CommandLine = argv[2];


    SERVICE_TABLE_ENTRY serviceTable[] =
    {
        { (LPWSTR)TEXT("WTSService"), ServiceMain },
        { NULL, NULL }
    };

    if (StartServiceCtrlDispatcher(serviceTable) == FALSE)
    {
        std::cerr << "StartServiceCtrlDispatcher failed." << std::endl;
        return GetLastError();
    }


    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR argv[])
{
    // Register the service control handler
    serviceStatusHandle = RegisterServiceCtrlHandler(TEXT("WTSService"), ServiceCtrlHandler);
    if (serviceStatusHandle == NULL)
    {
        std::cerr << "RegisterServiceCtrlHandler failed." << std::endl;
        return;
    }

    // Initialize the service status
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    serviceStatus.dwWin32ExitCode = 0;
    serviceStatus.dwServiceSpecificExitCode = 0;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;

    // Report the service status to the SCM
    if (SetServiceStatus(serviceStatusHandle, &serviceStatus) == FALSE)
    {
        std::cerr << "SetServiceStatus failed." << std::endl;
        return;
    }

    // Perform service initialization here

    // Change the service status to running
    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    ImpersonateExecute(SessionId, CommandLine);


    
    if (SetServiceStatus(serviceStatusHandle, &serviceStatus) == FALSE)
    {
        std::cerr << "SetServiceStatus failed." << std::endl;
        return;
    }



     Sleep(7*1000);


    // Change the service status to stopped
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    if (SetServiceStatus(serviceStatusHandle, &serviceStatus) == FALSE)
    {
        std::cerr << "SetServiceStatus failed." << std::endl;
        return;
    }
}

VOID WINAPI ServiceCtrlHandler(DWORD controlCode)
{
    switch (controlCode)
    {
    case SERVICE_CONTROL_STOP:
        // Perform cleanup and stop the service
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        if (SetServiceStatus(serviceStatusHandle, &serviceStatus) == FALSE)
        {
            std::cerr << "SetServiceStatus failed." << std::endl;
        }
        // Perform cleanup here

        // Change the service status to stopped
        serviceStatus.dwCurrentState = SERVICE_STOPPED;
        if (SetServiceStatus(serviceStatusHandle, &serviceStatus) == FALSE)
        {
            std::cerr << "SetServiceStatus failed." << std::endl;
        }
        break;

    default:
        break;
    }
}