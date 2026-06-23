#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RepoCommandLibrary.generated.h"

USTRUCT(BlueprintType)
struct FLauncherManifest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ManifestName;

    UPROPERTY(BlueprintReadOnly)
    FString ManifestDesc;

    UPROPERTY(BlueprintReadOnly)
    int64 ManifestVersion = 0;

    UPROPERTY(BlueprintReadOnly)
    FString MinSupportedVersion;

    UPROPERTY(BlueprintReadOnly)
    FString LatestVersion;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> GamesURLs;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> CustomMeta;
};

// Delegates for Blueprint-friendly callbacks
DECLARE_DYNAMIC_DELEGATE_TwoParams(FRepoLogDelegate, bool, bIsError, const FString&, Message);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSetRepoUrlDelegate, const FString&, BaseURL, const FString&, ManifestName);

UCLASS()
class PIOZAGAMELAUNCHER_API URepoCommandLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Main entry point for the setrepo command, optimized for Blueprint execution.
     */
    UFUNCTION(BlueprintCallable, Category = "Launcher | Commands", meta = (DisplayName = "Execute Set Repo"))
    static void ExecuteSetRepo(
        const TArray<FString>& CommandArgs,
        const FString& DefaultCdnUrl,
        FRepoLogDelegate LogCallback,
        FSetRepoUrlDelegate SetRepoUrlCallback);

private:
    static void DownloadManifest(const FString& TargetURL, TFunction<void(bool, const FString&)> Callback);
    static bool ParseManifest(const FString& JsonString, FLauncherManifest& OutManifest, FString& OutError);

    // Helper method to parse a single FJsonObject into FLauncherManifest
    static bool ParseJsonObject(const TSharedRef<FJsonObject>& JsonObject, FLauncherManifest& OutManifest);

    static void SplitURL(const FString& FullURL, FString& OutBaseURL, FString& OutManifestName);
    static FString CleanJsonString(const FString& RawString);
};