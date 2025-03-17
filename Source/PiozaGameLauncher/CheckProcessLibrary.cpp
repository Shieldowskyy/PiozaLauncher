#include "CheckProcessLibrary.h"

#if PLATFORM_WINDOWS
    #include "Windows/WindowsHWrapper.h"
    #include "Windows/AllowWindowsPlatformTypes.h"
    #include "Windows/WindowsPlatformProcess.h" // Required for working with processes
    #include <tlhelp32.h> // Provides functions for working with processes (e.g. EnumProcesses)
#elif PLATFORM_LINUX
    #include <dirent.h>   // For Linux process enumeration
    #include <cstring>    // For string comparison
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
    // Linux-specific code
    const char* processNameC = TCHAR_TO_UTF8(*ProcessName);

    // Open the /proc directory to get process IDs
    DIR* dir = opendir("/proc");
    if (dir == nullptr)
    {
        return false; // Failed to open /proc directory
    }

    struct dirent* entry;
    bool found = false;

    // Iterate over all entries in /proc (each entry is a process ID)
    while ((entry = readdir(dir)) != nullptr)
    {
        // Check if the entry name is a number (process ID)
        if (isdigit(entry->d_name[0]))
        {
            // Build the path to the command line for this process
            char cmdPath[256];
            snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", entry->d_name);

            // Open the cmdline file for this process
            FILE* file = fopen(cmdPath, "r");
            if (file)
            {
                char buffer[256];
                // Read the process name from the cmdline
                if (fgets(buffer, sizeof(buffer), file))
                {
                    // Compare the process name
                    if (strstr(buffer, processNameC) != nullptr)
                    {
                        found = true;
                        fclose(file);
                        break;
                    }
                }
                fclose(file);
            }
        }
    }

    closedir(dir);
    return found;
#else
    return false;
#endif
}