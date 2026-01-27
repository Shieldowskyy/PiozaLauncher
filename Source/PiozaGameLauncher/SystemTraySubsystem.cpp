// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "SystemTraySubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Async/Async.h"
#include "WindowUtils.h"
#include "Misc/Guid.h"

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
#include "StatusNotifierItemXML.h"

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

static void AddStringVariant(DBusMessageIter* iter, const char* val)
{
    DBusMessageIter variant;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
    dbus_message_iter_close_container(iter, &variant);
}

static void AddBoolVariant(DBusMessageIter* iter, dbus_bool_t val)
{
    DBusMessageIter variant;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "b", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &val);
    dbus_message_iter_close_container(iter, &variant);
}

static void AddUintVariant(DBusMessageIter* iter, uint32 val)
{
    DBusMessageIter variant;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "u", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_UINT32, &val);
    dbus_message_iter_close_container(iter, &variant);
}

static void Prop_Category(DBusMessageIter* iter, USystemTraySubsystem*) { AddStringVariant(iter, "ApplicationStatus"); }
static void Prop_Id(DBusMessageIter* iter, USystemTraySubsystem*) { AddStringVariant(iter, "PiozaLauncher"); }
static void Prop_Title(DBusMessageIter* iter, USystemTraySubsystem* Sub) {
    FString Val = Sub ? Sub->LastTooltip : TEXT("Pioza Launcher");
    FTCHARToUTF8 Conv(*Val);
    AddStringVariant(iter, (const char*)Conv.Get());
}
static void Prop_Status(DBusMessageIter* iter, USystemTraySubsystem*) { AddStringVariant(iter, "Active"); }
static void Prop_WindowId(DBusMessageIter* iter, USystemTraySubsystem*) { AddUintVariant(iter, 0); }
static void Prop_ItemIsMenu(DBusMessageIter* iter, USystemTraySubsystem*) { AddBoolVariant(iter, FALSE); }
static void Prop_IconName(DBusMessageIter* iter, USystemTraySubsystem* Sub) {
    FString Val = (Sub && !Sub->LastIconPath.IsEmpty()) ? FPaths::GetBaseFilename(Sub->LastIconPath) : TEXT("applications-other");
    FTCHARToUTF8 Conv(*Val);
    AddStringVariant(iter, (const char*)Conv.Get());
}
static void Prop_IconThemePath(DBusMessageIter* iter, USystemTraySubsystem* Sub) {
    FString Val = (Sub && !Sub->LastIconPath.IsEmpty()) ? FPaths::ConvertRelativePathToFull(FPaths::GetPath(Sub->LastIconPath)) : TEXT("");
    FTCHARToUTF8 Conv(*Val);
    AddStringVariant(iter, (const char*)Conv.Get());
}

static void Prop_Menu(DBusMessageIter* iter, USystemTraySubsystem*) { 
    FString Path = FString::Printf(TEXT("/MenuBar"));
    FTCHARToUTF8 Conv(*Path);
    const char* PathStr = (const char*)Conv.Get();
    
    DBusMessageIter variant;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "o", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH, &PathStr);
    dbus_message_iter_close_container(iter, &variant);
}

struct FDBusPropertyEntry {
    const char* Name;
    void (*Handler)(DBusMessageIter* iter, USystemTraySubsystem* Subsystem);
};

static const FDBusPropertyEntry DBusProperties[] = {
    { "Category", Prop_Category },
    { "Id", Prop_Id },
    { "Title", Prop_Title },
    { "Status", Prop_Status },
    { "WindowId", Prop_WindowId },
    { "IconName", Prop_IconName },
    { "IconThemePath", Prop_IconThemePath },
    { "ItemIsMenu", Prop_ItemIsMenu },
    { "Menu", Prop_Menu },
    { nullptr, nullptr }
};

void UTrayMenuItem::SetLabel(const FString& InLabel)
{
    Label = InLabel;
    NotifyParentRefresh();
}

void UTrayMenuItem::SetEnabled(bool bInEnabled)
{
    bIsEnabled = bInEnabled;
    NotifyParentRefresh();
}

void UTrayMenuItem::RemoveFromTray()
{
    if (USystemTraySubsystem* Parent = Cast<USystemTraySubsystem>(GetOuter()))
    {
        Parent->RemoveTrayMenuItem(this);
    }
}

void UTrayMenuItem::NotifyParentRefresh()
{
    if (USystemTraySubsystem* Parent = Cast<USystemTraySubsystem>(GetOuter()))
    {
        Parent->RefreshTrayMenu();
    }
}

static bool AppendPropertyVariant(DBusMessageIter* iter, const char* property, USystemTraySubsystem* Subsystem)
{
    if (!property) return false;
    
    for (const FDBusPropertyEntry* Entry = DBusProperties; Entry->Name; ++Entry) {
        if (strcmp(property, Entry->Name) == 0) {
            Entry->Handler(iter, Subsystem);
            return true;
        }
    }
    return false;
}

static DBusHandlerResult TrayDBusMessageHandler(::DBusConnection* conn, DBusMessage* msg, void* data)
{
    if (!conn || !msg) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
    USystemTraySubsystem* Subsystem = (USystemTraySubsystem*)data;
    
    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect"))
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &SNI_INTROSPECTION_XML, DBUS_TYPE_INVALID);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus_message_is_method_call(msg, "org.dashogames.piozalauncher", "Activate"))
    {
        if (Subsystem && IsValid(Subsystem))
        {
            TWeakObjectPtr<USystemTraySubsystem> WeakSubsystem = Subsystem;
            AsyncTask(ENamedThreads::GameThread, [WeakSubsystem]() {
                if (USystemTraySubsystem* SafeSubsystem = WeakSubsystem.Get())
                {
                    SafeSubsystem->OnTrayIconClickedNative.Broadcast();
                    SafeSubsystem->OnTrayIconClicked.Broadcast();
                    
                    if (FSlateApplication::IsInitialized())
                    {
                        TSharedPtr<GenericApplication> GenericApp = FSlateApplication::Get().GetPlatformApplication();
                        if (GenericApp.IsValid())
                        {
                            TSharedPtr<SWindow> TopWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
                            if (TopWindow.IsValid())
                            {
                                TopWindow->BringToFront(true);
                            }
                        }
                    }
                }
            });
        }
        
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus_message_is_method_call(msg, "org.kde.StatusNotifierItem", "Activate"))
    {
        if (Subsystem && IsValid(Subsystem)) 
        {
            TWeakObjectPtr<USystemTraySubsystem> WeakSubsystem = Subsystem;
            AsyncTask(ENamedThreads::GameThread, [WeakSubsystem]() {
                if (USystemTraySubsystem* SafeSubsystem = WeakSubsystem.Get())
                {
                    SafeSubsystem->OnTrayIconClicked.Broadcast();
                    SafeSubsystem->OnTrayIconClickedNative.Broadcast();
                }
            });
        }
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "Get"))
    {
        const char *iface_req = nullptr, *prop = nullptr;
        if (dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &iface_req, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID))
        {
            DBusMessage* reply = dbus_message_new_method_return(msg);
            if (reply)
            {
                DBusMessageIter iter;
                dbus_message_iter_init_append(reply, &iter);
                
                if (!AppendPropertyVariant(&iter, prop, Subsystem))
                {
                    AddStringVariant(&iter, "");
                }
                
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "GetAll"))
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            DBusMessageIter iter, dict;
            dbus_message_iter_init_append(reply, &iter);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
            
            for (const FDBusPropertyEntry* Entry = DBusProperties; Entry->Name; ++Entry)
            {
                DBusMessageIter entry_iter;
                dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &Entry->Name);
                Entry->Handler(&entry_iter, Subsystem);
                dbus_message_iter_close_container(&dict, &entry_iter);
            }
            
            dbus_message_iter_close_container(&iter, &dict);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void AppendMenuItem(DBusMessageIter* parent_array_iter, int id, const char* label, bool enabled, bool separator)
{
    DBusMessageIter variant, struct_iter;
    dbus_message_iter_open_container(parent_array_iter, DBUS_TYPE_VARIANT, "(ia{sv}av)", &variant);
    dbus_message_iter_open_container(&variant, DBUS_TYPE_STRUCT, nullptr, &struct_iter);
    
    dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &id);
    
    DBusMessageIter props_iter;
    dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &props_iter);
    
    if (label) {
        DBusMessageIter entry, val;
        const char* key = "label";
        dbus_message_iter_open_container(&props_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &val);
        dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &label);
        dbus_message_iter_close_container(&entry, &val);
        dbus_message_iter_close_container(&props_iter, &entry);
    }
    
    if (!enabled) {
        DBusMessageIter entry, val;
        const char* key = "enabled";
        dbus_bool_t b = FALSE;
        dbus_message_iter_open_container(&props_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &val);
        dbus_message_iter_append_basic(&val, DBUS_TYPE_BOOLEAN, &b);
        dbus_message_iter_close_container(&entry, &val);
        dbus_message_iter_close_container(&props_iter, &entry);
    }
    
    if (separator) {
        DBusMessageIter entry, val;
        const char* key = "type";
        const char* type_val = "separator";
        dbus_message_iter_open_container(&props_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &val);
        dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &type_val);
        dbus_message_iter_close_container(&entry, &val);
        dbus_message_iter_close_container(&props_iter, &entry);
    }

    dbus_message_iter_close_container(&struct_iter, &props_iter);
    
    DBusMessageIter children_iter;
    dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "v", &children_iter);
    dbus_message_iter_close_container(&struct_iter, &children_iter);
    
    dbus_message_iter_close_container(&variant, &struct_iter);
    dbus_message_iter_close_container(parent_array_iter, &variant);
}

static DBusHandlerResult MenuDBusMessageHandler(::DBusConnection* conn, DBusMessage* msg, void* data)
{
    if (!conn || !msg) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
    USystemTraySubsystem* Subsystem = (USystemTraySubsystem*)data;
    
    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect"))
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &DBUSMENU_INTROSPECTION_XML, DBUS_TYPE_INVALID);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "GetLayout"))
    {
        int parentId = 0, recursionDepth = -1;
        
        DBusMessageIter args;
        if (dbus_message_iter_init(msg, &args)) {
            dbus_message_iter_get_basic(&args, &parentId);
            dbus_message_iter_next(&args);
            dbus_message_iter_get_basic(&args, &recursionDepth);
        }
        
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            DBusMessageIter iter;
            dbus_message_iter_init_append(reply, &iter);
            
            uint32 revision = 1;
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &revision);
            
            DBusMessageIter root_struct;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, nullptr, &root_struct);
            
            int root_id = 0;
            dbus_message_iter_append_basic(&root_struct, DBUS_TYPE_INT32, &root_id);
            
            DBusMessageIter root_props;
            dbus_message_iter_open_container(&root_struct, DBUS_TYPE_ARRAY, "{sv}", &root_props);
            {
                 const char* k = "children-display"; const char* v = "submenu";
                 DBusMessageIter e, val;
                 dbus_message_iter_open_container(&root_props, DBUS_TYPE_DICT_ENTRY, nullptr, &e);
                 dbus_message_iter_append_basic(&e, DBUS_TYPE_STRING, &k);
                 dbus_message_iter_open_container(&e, DBUS_TYPE_VARIANT, "s", &val);
                 dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &v);
                 dbus_message_iter_close_container(&e, &val);
                 dbus_message_iter_close_container(&root_props, &e);
            }
            dbus_message_iter_close_container(&root_struct, &root_props);
            
            DBusMessageIter children_array;
            dbus_message_iter_open_container(&root_struct, DBUS_TYPE_ARRAY, "v", &children_array);
            
            if (parentId == 0) {
                if (Subsystem && IsValid(Subsystem))
                {
                    for (UTrayMenuItem* Item : Subsystem->MenuItems)
                    {
                        if (Item && IsValid(Item))
                        {
                            FTCHARToUTF8 Conv(*Item->Label);
                            AppendMenuItem(&children_array, Item->InternalId, Item->bIsSeparator ? nullptr : (const char*)Conv.Get(), Item->bIsEnabled, Item->bIsSeparator);
                        }
                    }
                }
            }
            
            dbus_message_iter_close_container(&root_struct, &children_array);
            dbus_message_iter_close_container(&iter, &root_struct);
            
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "GetGroupProperties"))
    {
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            DBusMessageIter iter, arr;
            dbus_message_iter_init_append(reply, &iter);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(ia{sv})", &arr);
            dbus_message_iter_close_container(&iter, &arr);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "Event"))
    {
        int id = 0;
        const char* eventId = nullptr;
        DBusMessageIter args;
        if (dbus_message_iter_init(msg, &args)) {
            dbus_message_iter_get_basic(&args, &id);
            dbus_message_iter_next(&args);
            dbus_message_iter_get_basic(&args, &eventId);
        }
        
        if (eventId && strcmp(eventId, "clicked") == 0) {
            UE_LOG(LogTemp, Log, TEXT("Tray Menu Clicked: %d"), id);
            if (Subsystem && IsValid(Subsystem)) {
                TWeakObjectPtr<USystemTraySubsystem> WeakSubsystem = Subsystem;
                AsyncTask(ENamedThreads::GameThread, [WeakSubsystem, id]() {
                    if (USystemTraySubsystem* SafeSubsystem = WeakSubsystem.Get())
                    {
                        for (UTrayMenuItem* Item : SafeSubsystem->MenuItems)
                        {
                            if (Item && IsValid(Item) && Item->InternalId == id)
                            {
                                Item->OnClicked.Broadcast(Item);
                                break;
                            }
                        }
                        
                        if (id == 3) {
                            SafeSubsystem->OnTrayIconClicked.Broadcast();
                            SafeSubsystem->OnTrayIconClickedNative.Broadcast();
                        }
                        else if (id == 4) {
                            FPlatformMisc::RequestExit(false);
                        }
                    }
                });
            }
        }
        
        DBusMessage* reply = dbus_message_new_method_return(msg);
        if (reply)
        {
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "AboutToShow"))
    {
         DBusMessage* reply = dbus_message_new_method_return(msg);
         if (reply)
         {
             dbus_bool_t b = FALSE;
             dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &b, DBUS_TYPE_INVALID);
             dbus_connection_send(conn, reply, nullptr);
             dbus_message_unref(reply);
         }
         return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "Get")) {
          DBusMessage* reply = dbus_message_new_method_return(msg);
          if (reply)
          {
              DBusMessageIter iter, v;
              dbus_message_iter_init_append(reply, &iter);
              dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &v);
              const char* s = "";
              dbus_message_iter_append_basic(&v, DBUS_TYPE_STRING, &s);
              dbus_message_iter_close_container(&iter, &v);
              dbus_connection_send(conn, reply, nullptr);
              dbus_message_unref(reply);
          }
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

    AddTrayMenuItem(CreateTrayMenuItem(TEXT("Pioza Launcher"), false, false));
    AddTrayMenuItem(CreateTrayMenuItem(TEXT(""), true, true));
    AddTrayMenuItem(CreateTrayMenuItem(TEXT("Open Pioza Launcher")));
    AddTrayMenuItem(CreateTrayMenuItem(TEXT("Quit Pioza Launcher")));

    if (!TickerHandle.IsValid())
    {
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USystemTraySubsystem::TickSubsystem), 0.1f);
    }
}

FString USystemTraySubsystem::GetBestIconPath() const
{
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
    IconPath = FPaths::ConvertRelativePathToFull(IconPath);
#endif
    return IconPath;
}

bool USystemTraySubsystem::IsApplicationInTray() const
{
    return bIsIconVisible;
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
    LastTooltip = Tooltip;
    if (!IconPath.IsEmpty())
    {
        LastIconPath = IconPath;
    }

    if (bIsIconVisible)
    {
#if PLATFORM_WINDOWS
        if (TrayIconData)
        {
            NOTIFYICONDATA* nid = (NOTIFYICONDATA*)TrayIconData;
            FCString::Strncpy(nid->szTip, *Tooltip, sizeof(nid->szTip) / sizeof(nid->szTip[0]));
            nid->szTip[(sizeof(nid->szTip) / sizeof(nid->szTip[0])) - 1] = '\0';
            Shell_NotifyIcon(NIM_MODIFY, nid);
        }
#endif
#if PLATFORM_LINUX
        if (NativeDBusConnection) {
             ::DBusConnection* conn = (::DBusConnection*)NativeDBusConnection;
             
             DBusMessage* signal = dbus_message_new_signal("/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewIcon");
             if (signal)
             {
                 dbus_connection_send(conn, signal, nullptr);
                 dbus_message_unref(signal);
             }
             
             signal = dbus_message_new_signal("/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewTitle");
             if (signal)
             {
                 dbus_connection_send(conn, signal, nullptr);
                 dbus_message_unref(signal);
             }

             signal = dbus_message_new_signal("/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewStatus");
             if (signal)
             {
                 dbus_connection_send(conn, signal, nullptr);
                 dbus_message_unref(signal);
             }
             
             dbus_connection_flush(conn);
        }
#endif
        return;
    }

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
    
    FString FullPath = IconPath;
    if (FullPath.IsEmpty())
    {
        FullPath = GetBestIconPath();
    }
    
    if (!FullPath.IsEmpty() && FPaths::FileExists(FullPath))
    {
        nid->hIcon = (HICON)LoadImage(NULL, *FPaths::ConvertRelativePathToFull(FullPath), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
    
    if (!nid->hIcon)
    {
        nid->hIcon = ExtractIcon(GetModuleHandle(NULL), *FPlatformProcess::ExecutablePath(), 0);
    }
    
    if (!nid->hIcon)
    {
        nid->hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    FCString::Strncpy(nid->szTip, *Tooltip, sizeof(nid->szTip) / sizeof(nid->szTip[0]));
    nid->szTip[(sizeof(nid->szTip) / sizeof(nid->szTip[0])) - 1] = '\0';
    
    if (Shell_NotifyIcon(NIM_ADD, nid))
    {
        TrayIconData = nid;
        bIsIconVisible = true;
        OnApplicationMinimizedToTray.Broadcast();
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
            FString BusName = FString::Printf(TEXT("org.kde.StatusNotifierItem-%d-%s"), getpid(), *FGuid::NewGuid().ToString());
            FTCHARToUTF8 Converter(*BusName);
            const char* service_ptr = (const char*)Converter.Get();
            
            if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &service_ptr, DBUS_TYPE_INVALID))
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to append args to DBus message"));
                dbus_message_unref(msg);
                return;
            }
            
            DBusError error;
            dbus_error_init(&error);
            
            DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 1000, &error);
            
            if (dbus_error_is_set(&error))
            {
                UE_LOG(LogTemp, Error, TEXT("DBus error: %s - %s"), 
                       UTF8_TO_TCHAR(error.name), UTF8_TO_TCHAR(error.message));
                dbus_error_free(&error);
                dbus_message_unref(msg);
                return;
            }
            
            if (reply)
            {
                bIsIconVisible = true;
                OnApplicationMinimizedToTray.Broadcast();
                UE_LOG(LogTemp, Log, TEXT("Successfully registered StatusNotifierItem: %s"), *BusName);
                
                dbus_message_unref(reply);
                
                DBusMessage* signal = dbus_message_new_signal("/StatusNotifierItem", 
                                                             "org.kde.StatusNotifierItem", 
                                                             "NewIcon");
                if (signal)
                {
                    dbus_connection_send(conn, signal, nullptr);
                    dbus_message_unref(signal);
                    dbus_connection_flush(conn);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to register StatusNotifierItem - no reply"));
            }
            
            dbus_message_unref(msg);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create DBus message"));
        }
    }
#endif
}

UTrayMenuItem* USystemTraySubsystem::CreateTrayMenuItem(const FString& Label, bool bIsEnabled, bool bIsSeparator)
{
    UTrayMenuItem* NewItem = NewObject<UTrayMenuItem>(this);
    NewItem->InternalId = NextInternalId++;
    NewItem->Label = Label;
    NewItem->bIsEnabled = bIsEnabled;
    NewItem->bIsSeparator = bIsSeparator;
    return NewItem;
}

void USystemTraySubsystem::AddTrayMenuItem(UTrayMenuItem* MenuItem)
{
    InsertTrayMenuItem(MenuItem, MenuItems.Num());
}

void USystemTraySubsystem::InsertTrayMenuItem(UTrayMenuItem* MenuItem, int32 Index)
{
    if (MenuItem && !MenuItems.Contains(MenuItem))
    {
        Index = FMath::Clamp(Index, 0, MenuItems.Num());
        MenuItems.Insert(MenuItem, Index);
        RefreshTrayMenu();
    }
}

void USystemTraySubsystem::RemoveTrayMenuItem(UTrayMenuItem* MenuItem)
{
    if (MenuItem)
    {
        MenuItems.Remove(MenuItem);
        RefreshTrayMenu();
    }
}

void USystemTraySubsystem::ClearTrayMenuItems(bool bKeepTitle)
{
    if (bKeepTitle)
    {
        MenuItems.RemoveAll([](UTrayMenuItem* Item) {
            return Item && Item->InternalId != 1 && Item->InternalId != 2;
        });
    }
    else
    {
        MenuItems.Empty();
    }

    RefreshTrayMenu();
}

void USystemTraySubsystem::RefreshTrayMenu()
{
#if PLATFORM_LINUX
    if (NativeDBusConnection) {
        ::DBusConnection* conn = (::DBusConnection*)NativeDBusConnection;
        DBusMessage* signal = dbus_message_new_signal("/MenuBar", "com.canonical.dbusmenu", "LayoutUpdated");
        if (signal)
        {
            dbus_connection_send(conn, signal, nullptr);
            dbus_message_unref(signal);
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
    OnApplicationRestoredFromTray.Broadcast();
}

void USystemTraySubsystem::OnRequestDestroyWindowOverride(const TSharedRef<SWindow>& InWindow)
{
    UE_LOG(LogTemp, Log, TEXT("Window close requested, minimizing to tray instead."));
    
    FString IconPath = GetBestIconPath();
    
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
        else if (TrayEvent == WM_RBUTTONUP)
        {
            ShowWindowsContextMenu(hWnd);
            return true;
        }
    }
    else if (Message == WM_COMMAND)
    {
        int32 Id = LOWORD(WParam);
        for (UTrayMenuItem* Item : MenuItems)
        {
            if (Item && IsValid(Item) && Item->InternalId == Id)
            {
                Item->OnClicked.Broadcast(Item);
                
                if (Id == 3) {
                     OnTrayIconClicked.Broadcast();
                     OnTrayIconClickedNative.Broadcast();
                }
                else if (Id == 4) {
                     FPlatformMisc::RequestExit(false);
                }
                return true;
            }
        }
    }
    return false;
}

void USystemTraySubsystem::ShowWindowsContextMenu(void* hWnd)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    for (UTrayMenuItem* Item : MenuItems)
    {
        if (!Item || !IsValid(Item)) continue;

        uint32 Flags = MF_STRING;
        if (!Item->bIsEnabled) Flags |= MF_GRAYED;
        if (Item->bIsSeparator) Flags = MF_SEPARATOR;

        AppendMenu(hMenu, Flags, (UINT_PTR)Item->InternalId, *Item->Label);
    }

    POINT MousePos;
    GetCursorPos(&MousePos);

    SetForegroundWindow((HWND)hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, MousePos.x, MousePos.y, 0, (HWND)hWnd, NULL);
    DestroyMenu(hMenu);
}
#endif

#if PLATFORM_LINUX
void USystemTraySubsystem::InitLinuxDBus()
{
    DBusError err;
    dbus_error_init(&err);

    ::DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        UE_LOG(LogTemp, Warning, TEXT("DBus Connection Error (%s)"), UTF8_TO_TCHAR(err.message));
        dbus_error_free(&err);
        return;
    }
    
    if (!conn) return;

    const char* AppDBusName = "org.dashogames.piozalauncher";
    int app_result = dbus_bus_request_name(conn, AppDBusName, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
    
    if (dbus_error_is_set(&err)) {
        UE_LOG(LogTemp, Warning, TEXT("DBus Request Name Error for Single Instance (%s)"), UTF8_TO_TCHAR(err.message));
        dbus_error_free(&err);
    }
    
    if (app_result == DBUS_REQUEST_NAME_REPLY_EXISTS) {
        UE_LOG(LogTemp, Warning, TEXT("PiozaLauncher is already running. Activating existing instance..."));
        
        DBusMessage* msg = dbus_message_new_method_call(AppDBusName, "/Application", "org.dashogames.piozalauncher", "Activate");
        if (msg) {
            dbus_connection_send(conn, msg, nullptr);
            dbus_message_unref(msg);
            dbus_connection_flush(conn);
        }

        FPlatformMisc::RequestExit(false);
        return;
    }
    
    DBusObjectPathVTable app_vtable = {};
    app_vtable.message_function = TrayDBusMessageHandler;
    dbus_connection_register_object_path(conn, "/Application", &app_vtable, this);
    UE_LOG(LogTemp, Log, TEXT("DBus single instance registered: %s"), UTF8_TO_TCHAR(AppDBusName));

    FString Name = FString::Printf(TEXT("org.kde.StatusNotifierItem-%d-%s"), getpid(), *FGuid::NewGuid().ToString());
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
    
    DBusObjectPathVTable menu_vtable = {};
    menu_vtable.message_function = MenuDBusMessageHandler;
    if (dbus_connection_register_object_path(conn, "/MenuBar", &menu_vtable, this))
    {
         UE_LOG(LogTemp, Log, TEXT("Registered DBus object path /MenuBar"));
    }
}

void USystemTraySubsystem::ShutdownLinuxDBus()
{
    if (NativeDBusConnection)
    {
        ::DBusConnection* conn = (::DBusConnection*)NativeDBusConnection;
        dbus_connection_unregister_object_path(conn, "/StatusNotifierItem");
        dbus_connection_unregister_object_path(conn, "/MenuBar");
        dbus_connection_unregister_object_path(conn, "/Application");
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
        dbus_connection_read_write_dispatch(conn, 0);
    }
#endif

    if (FSlateApplication::IsInitialized())
    {
        TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
        if (MainWindow.IsValid())
        {
            MainWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateUObject(this, &USystemTraySubsystem::OnRequestDestroyWindowOverride));
            
            if (!bHasShownInitialTrayIcon && MainWindow->GetNativeWindow().IsValid())
            {
                bHasShownInitialTrayIcon = true;
                ShowTrayIcon(TEXT("Pioza Launcher"), GetBestIconPath());
                
                if (!OnTrayIconClickedNative.IsBound())
                {
                    OnTrayIconClickedNative.AddStatic(&UWindowUtils::RestoreFromTray);
                }
            }
        }
    }

    return true;
}