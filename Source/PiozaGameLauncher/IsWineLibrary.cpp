#include "IsWineLibrary.h"
#include "Windows/WindowsHWrapper.h"

bool UIsWineLibrary::IsRunningUnderWine()
{
#if PLATFORM_WINDOWS
	// Get a handle to ntdll.dll (this library exists in both Windows and Wine)
	HMODULE ntdll = GetModuleHandle(TEXT("ntdll.dll"));
	if (ntdll)
	{
		// Wine adds its own function wine_get_version in ntdll.dll
		if (GetProcAddress(ntdll, "wine_get_version"))
		{
			return true;
		}
	}
#endif
	return false;
}
