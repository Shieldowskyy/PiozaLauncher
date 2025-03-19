// Fill out your copyright notice in the Description page of Project Settings.


#include "GetAndSetRHI.h"
#include "HardwareInfo.h"


FString UGetAndSetRHI::GetCurrentRhiName(void)
{
	FString rhi = FHardwareInfo::GetHardwareInfo(NAME_RHI);
	return rhi;
}