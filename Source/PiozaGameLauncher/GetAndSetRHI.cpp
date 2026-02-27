// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "GetAndSetRHI.h"
#include "HardwareInfo.h"
#include "RHI.h"

FString UGetAndSetRHI::GetCurrentRhiName(void)
{
	FString rhi = FHardwareInfo::GetHardwareInfo(NAME_RHI);
	return rhi;
}

int32 UGetAndSetRHI::GetCurrentMonitorRefreshRate()
{
	FScreenResolutionArray Resolutions;
	if (RHIGetAvailableResolutions(Resolutions, false))
	{
		// On many platforms, the first or last entry with the current desktop resolution
		// will contain the current refresh rate. 
		// Since we want the "current" one, we can try to matches GEngine's current settings
		// or just return the highest one available for the "primary" mode.
		
		int32 MaxRefreshRate = 0;
		for (const FScreenResolutionRHI& Res : Resolutions)
		{
			if (Res.RefreshRate > (uint32)MaxRefreshRate)
			{
				MaxRefreshRate = Res.RefreshRate;
			}
		}

		if (MaxRefreshRate > 0)
		{
			return MaxRefreshRate;
		}
	}

	return 60;
}