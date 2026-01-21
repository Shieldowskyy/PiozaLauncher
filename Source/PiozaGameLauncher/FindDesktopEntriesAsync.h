// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "DesktopParser.h"
#include "FindDesktopEntriesAsync.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDesktopEntriesFound, const TArray<FDesktopEntryInfo>&, Entries);

/**
 * Async node to find desktop entries without blocking the game thread
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UFindDesktopEntriesAsync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnDesktopEntriesFound Completed;

	/**
	 * Scans specified paths for desktop entries asynchronously.
	 * 
	 * @param SearchPaths List of directories to scan.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Desktop Parser")
	static UFindDesktopEntriesAsync* FindDesktopEntriesAsync(UObject* WorldContextObject, const TArray<FString>& SearchPaths);

	virtual void Activate() override;

private:
	TArray<FString> SearchPaths;
	UObject* WorldContextObject;
};
