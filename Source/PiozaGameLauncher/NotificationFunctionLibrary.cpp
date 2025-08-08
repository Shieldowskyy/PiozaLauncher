#include "NotificationFunctionLibrary.h"
#include "Misc/Paths.h"
#include "Misc/OutputDeviceNull.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#endif

#if PLATFORM_LINUX
#include <stdlib.h>
#include <iostream>
#include <fstream>
#endif

void UNotificationFunctionLibrary::SendSystemNotification(const FString& Title, const FString& Message, const FString& AppName)
{
    // On Windows, we use Windows API for notification
#if PLATFORM_WINDOWS
    // Creating a system notification
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcscpy_s(nid.szInfoTitle, *Title);
    wcscpy_s(nid.szInfo, *Message);
    nid.hWnd = nullptr;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Setting the icon
    nid.uFlags = NIF_ICON;
    nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
#endif

    // On Linux we use notify-send utility to send a notification
#if PLATFORM_LINUX
    // Check if the user provided an application name
    FString Command;
    if (AppName.IsEmpty())
    {
        Command = FString::Printf(TEXT("notify-send '%s' '%s'"), *Title, *Message);
    }
    else
    {
        Command = FString::Printf(TEXT("notify-send --app-name='%s' '%s' '%s'"), *AppName, *Title, *Message);
    }

    // Debug: Save the command to logs
    UE_LOG(LogTemp, Log, TEXT("Linux command: %s"), *Command);

    // Debug: Print the command to the console
    std::cout << "Linux Command: " << TCHAR_TO_ANSI(*Command) << std::endl;

    // Run the notify-send command
    int32 Result = system(TCHAR_TO_ANSI(*Command));

    // Debug: Check the result of the command execution
    if (Result != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("notify-send command failed with error code: %d"), Result);
        std::cout << "Error: notify-send command failed with code " << Result << std::endl;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("notify-send command executed successfully."));
        std::cout << "Success: notify-send command executed." << std::endl;
    }
#endif
}
