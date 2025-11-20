#include "ChecksumLibraryAsync.h"
#include "ChecksumLibrary.h"
#include "Async/Async.h"

bool UChecksumLibraryAsync::VerifyFileChecksumAsync(const FString& FilePath,
													const FString& ExpectedChecksum,
													EChecksumAlgorithm Algorithm,
													const FVerifyChecksumResult& ResultCallback)
{
	// Guard against a null delegate – we still want to queue the work.
	if (!ResultCallback.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("VerifyFileChecksumAsync called without a bound delegate"));
	}

	// Queue the heavy work on a background thread.
	Async(EAsyncExecution::ThreadPool, [FilePath, ExpectedChecksum, Algorithm, ResultCallback]()
	{
		FString ActualChecksum;
		bool bCalcOk = UChecksumLibrary::CalculateFileChecksum(FilePath, Algorithm, ActualChecksum);
		bool bMatch   = false;

		if (bCalcOk)
		{
			bMatch = ActualChecksum.Equals(ExpectedChecksum, ESearchCase::IgnoreCase);
		}

		// Switch back to the game thread to fire the delegate.
		AsyncTask(ENamedThreads::GameThread, [ResultCallback, bCalcOk, bMatch]()
		{
			ResultCallback.ExecuteIfBound(bCalcOk, bMatch);
		});
	});

	return true; // task was queued
}
