// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "GetAndSetRHI.h"
#include "HardwareInfo.h"

FString UGetAndSetRHI::GetCurrentRhiName(void)
{
	FString rhi = FHardwareInfo::GetHardwareInfo(NAME_RHI);
	return rhi;
}