// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "ProcessTrackerLibrary.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#endif

TMap<int32, FProcHandle> UProcessTrackerLibrary::ActiveProcesses;
TMap<int32, TSet<uint32>> UProcessTrackerLibrary::TrackedProcessTrees;
TMap<uint32, TArray<uint32>> UProcessTrackerLibrary::CachedParentMap;
double UProcessTrackerLibrary::LastScanTime = 0.0;

void UProcessTrackerLibrary::RegisterProcess(int32 ProcessID, FProcHandle Handle)
{
    // Clear old data if PID is being reused
    ClearTracking(ProcessID);

    ActiveProcesses.Add(ProcessID, Handle);
    TrackedProcessTrees.FindOrAdd(ProcessID).Add((uint32)ProcessID);
}

bool UProcessTrackerLibrary::GetTrackedTree(int32 ProcessID, TSet<uint32>& OutTree)
{
    UpdateProcessTree(ProcessID);
    TSet<uint32>* Tree = TrackedProcessTrees.Find(ProcessID);
    if (Tree)
    {
        OutTree = *Tree;
        return true;
    }
    return false;
}

void UProcessTrackerLibrary::ClearTracking(int32 ProcessID)
{
    TrackedProcessTrees.Remove(ProcessID);
    ActiveProcesses.Remove(ProcessID);
}

void UProcessTrackerLibrary::UpdateProcessTree(int32 RootPID)
{
    TSet<uint32>& Tree = TrackedProcessTrees.FindOrAdd(RootPID);

    // Ensure RootPID is always in its own tree
    Tree.Add((uint32)RootPID);

    double CurrentTime = FPlatformTime::Seconds();
    // Cache the system scan for 0.5 seconds to avoid heavy load if called multi-frame or for multiple processes
    if (CurrentTime - LastScanTime > 0.5)
    {
        CachedParentMap.Empty();
        LastScanTime = CurrentTime;

        #if PLATFORM_WINDOWS
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (Snapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 Entry;
            Entry.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(Snapshot, &Entry))
            {
                do {
                    CachedParentMap.FindOrAdd(Entry.th32ParentProcessID).Add(Entry.th32ProcessID);
                } while (Process32Next(Snapshot, &Entry));
            }
            CloseHandle(Snapshot);
        }
        #elif PLATFORM_LINUX
        DIR* Dir = opendir("/proc");
        if (Dir)
        {
            struct dirent* Entry;
            while ((Entry = readdir(Dir)) != nullptr)
            {
                // Only check numeric directories
                const char* p = Entry->d_name;
                while (*p >= '0' && *p <= '9') p++;
                if (*p != '\0' || Entry->d_name[0] == '\0') continue;

                uint32 PID = (uint32)atoi(Entry->d_name);
                if (PID == 0) continue;

                std::ifstream StatFile("/proc/" + std::string(Entry->d_name) + "/stat");
                if (StatFile.is_open())
                {
                    std::string Line;
                    if (std::getline(StatFile, Line))
                    {
                        // Format: pid (comm) state ppid ...
                        size_t LastParen = Line.rfind(')');
                        if (LastParen != std::string::npos && LastParen + 1 < Line.length())
                        {
                            std::istringstream Stream(Line.substr(LastParen + 1));
                            std::string StateChar;
                            uint32 PPID = 0;
                            
                            if (Stream >> StateChar >> PPID)
                            {
                                CachedParentMap.FindOrAdd(PPID).Add(PID);
                            }
                        }
                    }
                }
            }
            closedir(Dir);
        }
        #endif
    }

    // Now perform BFS to find all descendants starting from RootPID
    TArray<uint32> Queue;
    Queue.Add((uint32)RootPID);
    
    int32 Head = 0;
    while (Head < Queue.Num())
    {
        uint32 CurrentPID = Queue[Head++];
        TArray<uint32>* Children = CachedParentMap.Find(CurrentPID);
        if (Children)
        {
            for (uint32 ChildPID : *Children)
            {
                if (!Tree.Contains(ChildPID))
                {
                    Tree.Add(ChildPID);
                    Queue.Add(ChildPID);
                }
            }
        }
    }
}

bool UProcessTrackerLibrary::IsProcessStillRunning(int32 ProcessID)
{
    UpdateProcessTree(ProcessID);

    TSet<uint32>* Tree = TrackedProcessTrees.Find(ProcessID);
    if (!Tree) return false;

    bool bAnyRunning = false;
    TArray<uint32> ToRemove;

    for (uint32 PID : *Tree)
    {
        FProcHandle Handle = FPlatformProcess::OpenProcess(PID);
        bool bIsRunning = false;
        if (Handle.IsValid())
        {
            bIsRunning = FPlatformProcess::IsProcRunning(Handle);
            
            #if PLATFORM_LINUX
            if (bIsRunning)
            {
                // Extra check for Zombie state
                std::ifstream StatFile("/proc/" + std::to_string(PID) + "/stat");
                if (StatFile.is_open())
                {
                    std::string Line;
                    if (std::getline(StatFile, Line))
                    {
                        size_t LastParen = Line.rfind(')');
                        if (LastParen != std::string::npos)
                        {
                            std::istringstream Stream(Line.substr(LastParen + 1));
                            std::string State;
                            if (Stream >> State)
                            {
                                if (State == "Z")
                                {
                                    bIsRunning = false;
                                }
                            }
                        }
                    }
                }
            }
            #endif

            FPlatformProcess::CloseProc(Handle);
        }

        if (bIsRunning)
        {
            bAnyRunning = true;
        }
        else
        {
            ToRemove.Add(PID);
        }
    }

    // Clean up dead PIDs to avoid false positives due to PID reuse
    for (uint32 DeadPID : ToRemove)
    {
        Tree->Remove(DeadPID);
    }

    if (!bAnyRunning)
    {
        TrackedProcessTrees.Remove(ProcessID);
        ActiveProcesses.Remove(ProcessID);
    }

    return bAnyRunning;
}

#if PLATFORM_WINDOWS
bool UProcessTrackerLibrary::GetProcessExecutablePath(uint32 PID, FString& OutPath)
{
    OutPath.Empty();

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, false, PID);
    if (!hProcess)
        return false;

    TCHAR Buffer[MAX_PATH] = { 0 };
    DWORD Size = MAX_PATH;

    if (GetModuleFileNameEx(hProcess, nullptr, Buffer, Size))
    {
        OutPath = FString(Buffer);
        CloseHandle(hProcess);
        return true;
    }

    CloseHandle(hProcess);
    return false;
}
#elif PLATFORM_LINUX
bool UProcessTrackerLibrary::GetProcessExecutablePath(int32 PID, FString& OutPath)
{
    OutPath.Empty();

    FString LinkPath = FString::Printf(TEXT("/proc/%d/exe"), PID);
    char Buffer[PATH_MAX];

    ssize_t Count = readlink(TCHAR_TO_ANSI(*LinkPath), Buffer, sizeof(Buffer) - 1);
    if (Count > 0)
    {
        Buffer[Count] = '\0';
        OutPath = FString(UTF8_TO_TCHAR(Buffer));
        return true;
    }

    return false;
}
#endif
