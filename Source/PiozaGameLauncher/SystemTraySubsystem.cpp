// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "SystemTraySubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Async/Async.h"
#include "WindowUtils.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <shellapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Windows/WindowsApplication.h"

#define WM_TRAYICON (WM_USER + 100)

USystemTraySubsystem* USystemTraySubsystem::Instance = nullptr;
#endif

#if PLATFORM_LINUX
#include <dbus/dbus.h>
#include <unistd.h>

static const char* SNI_INTROSPECTION_XML = 
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
    "    <method name=\"Get\">\n"
    "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"propname\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"value\" direction=\"out\" type=\"v\"/>\n"
    "    </method>\n"
    "    <method name=\"GetAll\">\n"
    "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"props\" direction=\"out\" type=\"a{sv}\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"org.kde.StatusNotifierItem\">\n"
    "    <method name=\"Activate\">\n"
    "      <arg name=\"x\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"y\" direction=\"in\" type=\"i\"/>\n"
    "    </method>\n"
    "    <method name=\"ContextMenu\">\n"
    "      <arg name=\"x\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"y\" direction=\"in\" type=\"i\"/>\n"
    "    </method>\n"
    "    <method name=\"Scroll\">\n"
    "      <arg name=\"delta\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"orientation\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <property name=\"Category\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"Id\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"Title\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"WindowId\" type=\"u\" access=\"read\"/>\n"
    "    <property name=\"IconName\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"IconThemePath\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/>\n"
    "  </interface>\n"
    "</node>\n";

static void AddProperty(DBusMessageIter* dict, const char* Key, const char* Type, const void* Value)
{
    DBusMessageIter entry, variant;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &Key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, Type, &variant);
    dbus_message_iter_append_basic(&variant, Type[0], Value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

static DBusHandlerResult TrayDBusMessageHandler(::DBusConnection* conn, DBusMessage* msg, void* data)
{
    USystemTraySubsystem* Subsystem = (USystemTraySubsystem*)data;
    
    // Introspection
    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect"))
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &SNI_INTROSPECTION_XML, DBUS_TYPE_INVALID);
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    // SNI Methods
    if (dbus_message_is_method_call(msg, "org.kde.StatusNotifierItem", "Activate"))
    {
        // Broadcast on UI thread
        if (Subsystem) 
        {
            Subsystem->OnTrayIconClicked.Broadcast();
            Subsystem->OnTrayIconClickedNative.Broadcast();
        }
        DBusMessage* reply = dbus_message_new_method_return(msg);
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    // Properties
    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "Get"))
    {
        const char *iface_req, *prop;
        if (dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &iface_req, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID))
        {
            UE_LOG(LogTemp, Log, TEXT("DBus Get Property: %s"), UTF8_TO_TCHAR(prop));
            DBusMessage* reply = dbus_message_new_method_return(msg);
            DBusMessageIter iter, variant;
            dbus_message_iter_init_append(reply, &iter);
            
            if (strcmp(prop, "Category") == 0) {
                const char* val = "ApplicationStatus";
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            } else if (strcmp(prop, "Id") == 0) {
                const char* val = "PiozaLauncher";
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            } else if (strcmp(prop, "Title") == 0) {
                FString Tooltip = Subsystem ? Subsystem->LastTooltip : TEXT("Pioza Launcher");
                FTCHARToUTF8 Converter(*Tooltip);
                const char* val = (const char*)Converter.Get();
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            } else if (strcmp(prop, "Status") == 0) {
                const char* val = "Active";
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            } else if (strcmp(prop, "IconName") == 0) {
                FString Icon = (Subsystem && !Subsystem->LastIconPath.IsEmpty()) ? FPaths::GetBaseFilename(Subsystem->LastIconPath) : TEXT("applications-other");
                FTCHARToUTF8 Converter(*Icon);
                const char* val = (const char*)Converter.Get();
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            } else if (strcmp(prop, "IconThemePath") == 0) {
                FString Path = (Subsystem && !Subsystem->LastIconPath.IsEmpty()) ? FPaths::ConvertRelativePathToFull(FPaths::GetPath(Subsystem->LastIconPath)) : TEXT("");
                FTCHARToUTF8 Converter(*Path);
                const char* val = (const char*)Converter.Get();
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            }
 else if (strcmp(prop, "ItemIsMenu") == 0) {
                dbus_bool_t val = FALSE;
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "b", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &val);
                dbus_message_iter_close_container(&iter, &variant);
            } else {
                const char* val = "";
                dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant);
                dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
                dbus_message_iter_close_container(&iter, &variant);
            }
            
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "GetAll"))
    {
        UE_LOG(LogTemp, Log, TEXT("DBus GetAll Properties requested"));
        DBusMessage* reply = dbus_message_new_method_return(msg);
        DBusMessageIter iter, dict;
        dbus_message_iter_init_append(reply, &iter);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
        
        const char* cat = "ApplicationStatus";
        const char* id = "PiozaLauncher";
        
        FString Tooltip = Subsystem ? Subsystem->LastTooltip : TEXT("Pioza Launcher");
        FTCHARToUTF8 TitleConv(*Tooltip);
        const char* title = (const char*)TitleConv.Get();
        const char* status = "Active";
        
        FString IconStr = (Subsystem && !Subsystem->LastIconPath.IsEmpty()) ? FPaths::GetBaseFilename(Subsystem->LastIconPath) : TEXT("applications-other");
        FTCHARToUTF8 IconConv(*IconStr);
        const char* icon = (const char*)IconConv.Get();
        
        FString ThemeStr = (Subsystem && !Subsystem->LastIconPath.IsEmpty()) ? FPaths::ConvertRelativePathToFull(FPaths::GetPath(Subsystem->LastIconPath)) : TEXT("");
        FTCHARToUTF8 ThemeConv(*ThemeStr);
        const char* theme = (const char*)ThemeConv.Get();

        dbus_bool_t menu = FALSE;
        uint32 winid = 0;

        AddProperty(&dict, "Category", "s", &cat);
        AddProperty(&dict, "Id", "s", &id);
        AddProperty(&dict, "Title", "s", &title);
        AddProperty(&dict, "Status", "s", &status);
        AddProperty(&dict, "IconName", "s", &icon);
        AddProperty(&dict, "IconThemePath", "s", &theme);
        AddProperty(&dict, "ItemIsMenu", "b", &menu);
        AddProperty(&dict, "WindowId", "u", &winid);

        dbus_message_iter_close_container(&iter, &dict);
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif

void USystemTraySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
#if PLATFORM_WINDOWS
    Instance = this;
    if (FSlateApplication::IsInitialized())
    {
        TSharedPtr<GenericApplication> GenericApp = FSlateApplication::Get().GetPlatformApplication();
        if (GenericApp.IsValid())
        {
            FWindowsApplication* WinApp = (FWindowsApplication*)GenericApp.Get();
            WinApp->AddMessageHandler(*this, &USystemTraySubsystem::HandleWindowsMessage);
        }
    }
#endif

#if PLATFORM_LINUX
    InitLinuxDBus();
#endif

    // Set up a ticker to hook into the main window's close request
    // This handles cases where the window is created after subsystem initialization
    if (!TickerHandle.IsValid())
    {
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USystemTraySubsystem::TickSubsystem), 0.1f);
    }
}

void USystemTraySubsystem::Deinitialize()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }

    HideTrayIcon();
    
#if PLATFORM_WINDOWS
    if (Instance == this) Instance = nullptr;
#endif

#if PLATFORM_LINUX
    ShutdownLinuxDBus();
#endif

    Super::Deinitialize();
}

void USystemTraySubsystem::ShowTrayIcon(const FString& Tooltip, const FString& IconPath)
{
    if (bIsIconVisible) return;
    
    LastTooltip = Tooltip;
    LastIconPath = IconPath;

#if PLATFORM_WINDOWS
    NOTIFYICONDATA* nid = new NOTIFYICONDATA();
    FMemory::Memzero(nid, sizeof(NOTIFYICONDATA));
    nid->cbSize = sizeof(NOTIFYICONDATA);
    
    TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid())
    {
        nid->hWnd = (HWND)MainWindow->GetNativeWindow()->GetOSWindowHandle();
    }
    
    nid->uID = 1;
    nid->uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid->uCallbackMessage = WM_TRAYICON;
    
    // Try to load custom icon
    FString FullPath = IconPath;
    if (FullPath.IsEmpty())
    {
        // Search in standard locations
        FullPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Icons"), TEXT("icon.ico"));
        if (!FPaths::FileExists(FullPath))
        {
            FullPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("pioza.ico"));
        }
    }
    
    if (!FullPath.IsEmpty() && FPaths::FileExists(FullPath))
    {
        nid->hIcon = (HICON)LoadImage(NULL, *FPaths::ConvertRelativePathToFull(FullPath), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
    
    // Fallback to application icon
    if (!nid->hIcon)
    {
        nid->hIcon = ExtractIcon(GetModuleHandle(NULL), *FPlatformProcess::ExecutablePath(), 0);
    }
    
    if (!nid->hIcon)
    {
        nid->hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    FCString::Strcpy(nid->szTip, 128, *Tooltip);
    
    if (Shell_NotifyIcon(NIM_ADD, nid))
    {
        TrayIconData = nid;
        bIsIconVisible = true;
    }
    else
    {
        if (nid->hIcon) DestroyIcon(nid->hIcon);
        delete nid;
    }
#endif

#if PLATFORM_LINUX
    if (NativeDBusConnection)
    {
        ::DBusConnection* conn = (::DBusConnection*)NativeDBusConnection;
        
        DBusMessage* msg = dbus_message_new_method_call("org.kde.StatusNotifierWatcher", 
                                                      "/StatusNotifierWatcher", 
                                                      "org.kde.StatusNotifierWatcher", 
                                                      "RegisterStatusNotifierItem");
        
        if (msg)
        {
            FString BusName = FString::Printf(TEXT("org.kde.StatusNotifierItem-%d-1"), getpid());
            FTCHARToUTF8 Converter(*BusName);
            const char* service_ptr = (const char*)Converter.Get();
            dbus_message_append_args(msg, DBUS_TYPE_STRING, &service_ptr, DBUS_TYPE_INVALID);
            
            if (dbus_connection_send(conn, msg, nullptr))
            {
                bIsIconVisible = true;
                UE_LOG(LogTemp, Log, TEXT("Sent RegisterStatusNotifierItem to DBus for %s"), *BusName);
                
                // Signal that the icon has changed (requested by some watchers)
                DBusMessage* signal = dbus_message_new_signal("/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewIcon");
                dbus_connection_send(conn, signal, nullptr);
                dbus_message_unref(signal);
            }
            dbus_message_unref(msg);
            dbus_connection_flush(conn);
        }
    }
#endif
}

void USystemTraySubsystem::HideTrayIcon()
{
    if (!bIsIconVisible) return;

#if PLATFORM_WINDOWS
    if (TrayIconData)
    {
        NOTIFYICONDATA* nid = (NOTIFYICONDATA*)TrayIconData;
        Shell_NotifyIcon(NIM_DELETE, nid);
        if (nid->hIcon) DestroyIcon(nid->hIcon);
        delete nid;
        TrayIconData = nullptr;
    }
#endif

    bIsIconVisible = false;
}

void USystemTraySubsystem::OnRequestDestroyWindowOverride(const TSharedRef<SWindow>& InWindow)
{
    UE_LOG(LogTemp, Log, TEXT("Window close requested, minimizing to tray instead."));
    
    // Search for icon in Content/Icons first for packaged builds
    FString IconPath = "";
    
#if PLATFORM_LINUX
    IconPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Icons"), TEXT("icon.png"));
    if (!FPaths::FileExists(IconPath)) {
        IconPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Build/Linux/AppImage/PiozaLauncher.AppDir/pioza_icon.png"));
    }
    IconPath = FPaths::ConvertRelativePathToFull(IconPath);
#else
    IconPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Icons"), TEXT("icon.ico"));
    if (!FPaths::FileExists(IconPath)) {
        IconPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("pioza.ico"));
    }
#endif
    
    UWindowUtils::MinimizeToTray(TEXT("Pioza Launcher is running in background"), IconPath);
}

#if PLATFORM_WINDOWS
bool USystemTraySubsystem::HandleWindowsMessage(void* hWnd, uint32 Message, uintptr_t WParam, intptr_t LParam, intptr_t* OutResult)
{
    if (Message == WM_TRAYICON)
    {
        uint32 TrayEvent = (uint32)LParam;
        if (TrayEvent == WM_LBUTTONUP || TrayEvent == WM_LBUTTONDBLCLK)
        {
            OnTrayIconClickedNative.Broadcast();
            OnTrayIconClicked.Broadcast();
            return true;
        }
    }
    return false;
}
#endif

#if PLATFORM_LINUX
void USystemTraySubsystem::InitLinuxDBus()
{
    DBusError err;
    dbus_error_init(&err);

    // Use shared connection to avoid threading issues with private ones
    ::DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        UE_LOG(LogTemp, Warning, TEXT("DBus Connection Error (%s)"), UTF8_TO_TCHAR(err.message));
        dbus_error_free(&err);
        return;
    }
    
    if (!conn) return;

    FString Name = FString::Printf(TEXT("org.kde.StatusNotifierItem-%d-1"), getpid());
    int result = dbus_bus_request_name(conn, TCHAR_TO_UTF8(*Name), DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        UE_LOG(LogTemp, Warning, TEXT("DBus Request Name Error (%s)"), UTF8_TO_TCHAR(err.message));
        dbus_error_free(&err);
    }
    
    if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        UE_LOG(LogTemp, Log, TEXT("DBus: Successfully became primary owner of %s"), *Name);
    } else {
        UE_LOG(LogTemp, Warning, TEXT("DBus: Name registration result for %s: %d"), *Name, result);
    }

    DBusObjectPathVTable vtable = {};
    vtable.message_function = TrayDBusMessageHandler;
    
    if (dbus_connection_register_object_path(conn, "/StatusNotifierItem", &vtable, this))
    {
        NativeDBusConnection = (void*)conn;
        UE_LOG(LogTemp, Log, TEXT("Registered DBus object path /StatusNotifierItem on %s"), *Name);
    }
}

void USystemTraySubsystem::ShutdownLinuxDBus()
{
    if (NativeDBusConnection)
    {
        ::DBusConnection* conn = (::DBusConnection*)NativeDBusConnection;
        dbus_connection_unregister_object_path(conn, "/StatusNotifierItem");
        NativeDBusConnection = nullptr;
    }
}
#endif

bool USystemTraySubsystem::TickSubsystem(float DeltaTime)
{
#if PLATFORM_LINUX
    if (NativeDBusConnection)
    {
        ::DBusConnection* conn = (::DBusConnection*)NativeDBusConnection;
        // Non-blocking dispatch
        dbus_connection_read_write_dispatch(conn, 0);
    }
#endif

    // Also use this ticker to ensure our window has the close hook
    if (FSlateApplication::IsInitialized())
    {
        TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
        if (MainWindow.IsValid())
        {
            // Intercept the close button by overriding the destroy request
            MainWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateUObject(this, &USystemTraySubsystem::OnRequestDestroyWindowOverride));
        }
    }

    return true; // Keep ticking
}
