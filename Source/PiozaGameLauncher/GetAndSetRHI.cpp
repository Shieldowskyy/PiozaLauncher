#include "GetAndSetRHI.h"
#include "HardwareInfo.h"
#include "RHI.h"

#if PLATFORM_LINUX
#include <cstdio>
#include <cstdlib>
#endif

FString UGetAndSetRHI::GetCurrentRhiName(void)
{
	FString rhi = FHardwareInfo::GetHardwareInfo(NAME_RHI);
	return rhi;
}

int32 UGetAndSetRHI::GetCurrentMonitorRefreshRate()
{
	#if PLATFORM_LINUX
	FILE* pipe = popen("xrandr | grep '\\*' | grep -oP '[0-9.]+\\*' | grep -oP '^[0-9]+' | sort -n | tail -1", "r");
	if (pipe)
	{
		char buffer[16] = {};
		fgets(buffer, sizeof(buffer), pipe);
		pclose(pipe);
		int32 rate = atoi(buffer);
		if (rate > 0) return rate;
	}
	return 60;
	#else
	FScreenResolutionArray Resolutions;
	if (RHIGetAvailableResolutions(Resolutions, false))
	{
		int32 MaxRefreshRate = 0;
		for (const FScreenResolutionRHI& Res : Resolutions)
		{
			if (Res.RefreshRate > (uint32)MaxRefreshRate)
				MaxRefreshRate = Res.RefreshRate;
		}
		if (MaxRefreshRate > 0) return MaxRefreshRate;
	}
	return 60;
	#endif
}