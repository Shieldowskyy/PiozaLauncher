#include "CheckProcessLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsPlatformProcess.h"
#include <tlhelp32.h>
#include <iostream>
#elif PLATFORM_LINUX
#include "HAL/PlatformProcess.h" // For FPlatformProcess
#endif

bool UCheckProcessLibrary::IsProcessRunning(const FString& ProcessName)
{
#if PLATFORM_WINDOWS
    // Windows-specific code
    std::wstring ProcessNameW(*ProcessName);

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            std::wstring CurrentProcessName = pe32.szExeFile;
            if (CurrentProcessName == ProcessNameW)
            {
                CloseHandle(hProcessSnap);
                return true;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return false;

#elif PLATFORM_LINUX
    // Convert FString to a format suitable for command execution
    FString Command = FString::Printf(TEXT("/bin/sh -c \"pidof %s\""), *ProcessName);
    FString Result;
    int32 ReturnCode;

    // Execute the command using FPlatformProcess::ExecProcess
    bool bSuccess = FPlatformProcess::ExecProcess(
        TEXT("/bin/sh"),          // Executable
        *Command,                 // Parameters
        &ReturnCode,              // Out return code
        &Result,                  // Out result
        nullptr                   // Optional working directory
    );

    // If the command executed successfully and pidof returned 0, the process is running
    return bSuccess && ReturnCode == 0;

#else
    return false;
#endif
}
