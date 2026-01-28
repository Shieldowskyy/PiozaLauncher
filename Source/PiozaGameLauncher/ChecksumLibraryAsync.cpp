// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "ChecksumLibraryAsync.h"
#include "ChecksumLibrary.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Misc/ScopeLock.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/Paths.h"

bool UChecksumLibraryAsync::VerifyFileChecksumAsync(const FString& FilePath,
													const FString& ExpectedChecksum,
													EChecksumAlgorithm Algorithm,
													const FVerifyChecksumResult& ResultCallback)
{
	if (!ResultCallback.IsBound()) return false;

	Async(EAsyncExecution::ThreadPool, [FilePath, ExpectedChecksum, Algorithm, ResultCallback]()
	{
		FString ActualChecksum;
		bool bCalcOk = UChecksumLibrary::CalculateFileChecksum(FilePath, Algorithm, ActualChecksum);
		bool bMatch = false;

		if (bCalcOk)
		{
			bMatch = ActualChecksum.Equals(ExpectedChecksum, ESearchCase::IgnoreCase);
		}

		AsyncTask(ENamedThreads::GameThread, [ResultCallback, bCalcOk, bMatch]()
		{
			ResultCallback.ExecuteIfBound(bCalcOk, bMatch);
		});
	});

	return true;
}

void UChecksumLibraryAsync::VerifyFileListAsync(const TArray<FString>& RelativeFilePaths,
												const TMap<FString, FString>& ExpectedChecksums,
												const FString& GameDirectory,
												const TArray<FString>& FilesToIgnore,
												EChecksumAlgorithm Algorithm,
												const FOnVerificationProgress& OnProgress,
												const FOnVerificationComplete& OnComplete)
{
	// Launch background task
	Async(EAsyncExecution::ThreadPool, [RelativeFilePaths, ExpectedChecksums, GameDirectory, FilesToIgnore, Algorithm, OnProgress, OnComplete]()
	{
		FVerificationResult Result;
		Result.TotalFilesChecked = 0;

		FThreadSafeCounter ProcessedCount(0);
		FCriticalSection ResultMutex;

		// Throttling settings
		double LastUpdateTime = FPlatformTime::Seconds();
		const double UpdateInterval = 0.05; // ~20 updates per second max

		ParallelFor(RelativeFilePaths.Num(), [&](int32 Index)
		{
			const FString& RelativePath = RelativeFilePaths[Index];

			// -----------------------------------------------------------------
			// 1. Check Ignored Files (Wildcard Support)
			// -----------------------------------------------------------------
			bool bIgnored = false;

			// Fast check
			if (FilesToIgnore.Contains(RelativePath))
			{
				bIgnored = true;
			}
			else
			{
				// Wildcard check (mimics Blueprint "MatchesAnyWildcard")
				for (const FString& Pattern : FilesToIgnore)
				{
					if (RelativePath.MatchesWildcard(Pattern, ESearchCase::IgnoreCase))
					{
						bIgnored = true;
						break;
					}
				}
			}

			if (bIgnored) return; // Skip this file
			// -----------------------------------------------------------------

			// 2. Construct Absolute Path
			FString FullFilePath = FPaths::Combine(GameDirectory, RelativePath);

			// 3. Calculate Checksum
			FString CalculatedChecksum;
			bool bCalcSuccess = UChecksumLibrary::CalculateFileChecksum(FullFilePath, Algorithm, CalculatedChecksum);

			// 4. Verify Logic
			bool bIsCorrupted = false;
			bool bIsMissing = !bCalcSuccess;

			if (bCalcSuccess)
			{
				const FString* Expected = ExpectedChecksums.Find(RelativePath);
				if (Expected)
				{
					if (!CalculatedChecksum.Equals(*Expected, ESearchCase::IgnoreCase))
					{
						bIsCorrupted = true;
					}
				}
				else
				{
					// File exists but is not in the expected map
					bIsCorrupted = true;
				}
			}

			// 5. Store Results (Thread-Safe)
			if (bIsMissing || bIsCorrupted)
			{
				FScopeLock Lock(&ResultMutex);
				if (bIsMissing)
				{
					Result.MissingFiles.Add(RelativePath);
				}
				else
				{
					Result.CorruptedFiles.Add(RelativePath);
				}
			}

			// 6. Update Progress (Throttled)
			int32 CurrentCount = ProcessedCount.Increment();

			double CurrentTime = FPlatformTime::Seconds();
			if (CurrentTime - LastUpdateTime > UpdateInterval)
			{
				// Non-blocking try-lock to prevent stalling worker threads
				if (ResultMutex.TryLock())
				{
					if (FPlatformTime::Seconds() - LastUpdateTime > UpdateInterval)
					{
						LastUpdateTime = FPlatformTime::Seconds();
						float Percent = (float)CurrentCount / (float)RelativeFilePaths.Num();
						FString FileForUI = RelativePath;

						AsyncTask(ENamedThreads::GameThread, [OnProgress, Percent, FileForUI]()
						{
							OnProgress.ExecuteIfBound(Percent, FileForUI);
						});
					}
					ResultMutex.Unlock();
				}
			}
		});

		// Set total files (approximate, based on input list size)
		Result.TotalFilesChecked = RelativeFilePaths.Num();

		// Final callback
		AsyncTask(ENamedThreads::GameThread, [OnComplete, Result]()
		{
			OnComplete.ExecuteIfBound(Result);
		});
	});
}
