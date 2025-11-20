#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChecksumLibraryAsync.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FVerifyChecksumResult,
	bool, bSuccess,
	bool, bMatch);

/**
 * Async Blueprint wrapper for VerifyFileChecksum
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UChecksumLibraryAsync : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Verify a file’s checksum without blocking the game thread.
	 *
	 * @param FilePath          Absolute path to the file.
	 * @param ExpectedChecksum  Expected checksum (case‑insensitive).
	 * @param Algorithm         Algorithm to use.
	 * @param ResultCallback    Called on the game thread when finished.
	 * @return true if the async task was successfully queued.
	 */
	UFUNCTION(BlueprintCallable,
		Category = "File|Checksum",
		meta = (AutoCreateRefTerm = "ResultCallback"))
	static bool VerifyFileChecksumAsync(const FString& FilePath,
										const FString& ExpectedChecksum,
										EChecksumAlgorithm Algorithm,
										const FVerifyChecksumResult& ResultCallback);
};
