// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "FindDesktopEntriesAsync.h"
#include "Async/Async.h"

UFindDesktopEntriesAsync* UFindDesktopEntriesAsync::FindDesktopEntriesAsync(UObject* WorldContextObject, const TArray<FString>& SearchPaths)
{
	UFindDesktopEntriesAsync* Action = NewObject<UFindDesktopEntriesAsync>();
	Action->WorldContextObject = WorldContextObject;
	Action->SearchPaths = SearchPaths;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UFindDesktopEntriesAsync::Activate()
{
	AddToRoot();

	if (SearchPaths.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindDesktopEntriesAsync: No search paths provided!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Starting scan with %d search paths."), SearchPaths.Num());
	}

	Async(EAsyncExecution::Thread, [this]()
	{
		UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Background thread started."));
		
		TArray<FString> FilesToParse;
		TArray<FString> DirectoriesToScan;

		for (const FString& Path : SearchPaths)
		{
			// Check if it looks like a desktop file/shortcut
			if (Path.EndsWith(TEXT(".desktop")) || Path.EndsWith(TEXT(".lnk")))
			{
				FilesToParse.Add(Path);
			}
			else
			{
				// Assume it's a directory (or verify existence if strictness is needed)
				DirectoriesToScan.Add(Path);
			}
		}

		if (FilesToParse.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Input contains %d direct file paths."), FilesToParse.Num());
		}

		if (DirectoriesToScan.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Scanning %d directories."), DirectoriesToScan.Num());
			TArray<FString> ScannedFiles = UDesktopParser::GetAllDesktopFiles(DirectoriesToScan);
			UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Scanned found %d files."), ScannedFiles.Num());
			FilesToParse.Append(ScannedFiles);
		}

		UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Total files to parse: %d"), FilesToParse.Num());

		TArray<FDesktopEntryInfo> Result = UDesktopParser::ParseMultipleDesktopFiles(FilesToParse);
		UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Parsed %d valid entries."), Result.Num());

		AsyncTask(ENamedThreads::GameThread, [this, Result]()
		{
			UE_LOG(LogTemp, Log, TEXT("FindDesktopEntriesAsync: Broadcasting results to Game Thread."));
			Completed.Broadcast(Result);
			RemoveFromRoot();
			SetReadyToDestroy();
		});
	});
}
