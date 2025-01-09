#include "CheckProcessLibrary.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsPlatformProcess.h" // Required for working with processes

#include <tlhelp32.h> // Provides functions for working with processes (e.g. EnumProcesses)

bool UCheckProcessLibrary::IsProcessRunning(const FString& ProcessName)
{
    // Only proceed if the platform is Windows
#if PLATFORM_WINDOWS
    // Convert FString to std::wstring (Windows API requires std::wstring type)
    std::wstring ProcessNameW(*ProcessName);  // Correct conversion from FString to std::wstring

    // Use Windows API function to create a snapshot of running processes
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    // Check if the snapshot creation was successful
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return false; // Failed to create snapshot, return false
    }

    // Retrieve the first process in the snapshot
    if (Process32First(hProcessSnap, &pe32))
    {
        // Loop through all processes in the snapshot
        do
        {
            // Compare the current process name with the specified one
            std::wstring CurrentProcessName = pe32.szExeFile;
            if (CurrentProcessName == ProcessNameW)
            {
                CloseHandle(hProcessSnap); // Close the process handle before returning
                return true; // Process found, return true
            }
        } while (Process32Next(hProcessSnap, &pe32)); // Move to the next process
    }

    // Close the process handle after enumerating all processes
    CloseHandle(hProcessSnap);
#endif

    // Return false if no matching process was found
    return false;
}
