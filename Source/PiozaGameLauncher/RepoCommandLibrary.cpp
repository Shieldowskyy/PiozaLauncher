#include "RepoCommandLibrary.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"

void URepoCommandLibrary::ExecuteSetRepo(
    const TArray<FString>& CommandArgs,
    const FString& DefaultCdnUrl,
    FRepoLogDelegate LogCallback,
    FSetRepoUrlDelegate SetRepoUrlCallback)
{
    if (CommandArgs.Num() < 2)
    {
        LogCallback.ExecuteIfBound(true, TEXT("WRONG USAGE! Usage: setrepo <default or reponame> OR <url> [--force]"));
        return;
    }

    const FString TargetArg = CommandArgs[1];
    const bool bForce = (CommandArgs.Num() >= 3 && CommandArgs[2].Equals(TEXT("--force"), ESearchCase::IgnoreCase));

    // 1. Default repository
    if (TargetArg.Equals(TEXT("default"), ESearchCase::IgnoreCase))
    {
        LogCallback.ExecuteIfBound(false, FString::Printf(TEXT("Setting repo url to default: %s"), *DefaultCdnUrl));
        SetRepoUrlCallback.ExecuteIfBound(DefaultCdnUrl, TEXT("games.json"));
        return;
    }

    // 2. Determine if argument is a URL or a Repository Name
    FString FinalURL;
    if (TargetArg.Contains(TEXT("http")) && TargetArg.Contains(TEXT("://")))
    {
        FinalURL = TargetArg;
    }
    else
    {
        FString ReplacedCdn = DefaultCdnUrl.Replace(TEXT("shipping"), *TargetArg, ESearchCase::IgnoreCase);
        FinalURL = ReplacedCdn + TEXT("games.json");
    }

    FString BaseURL, ManifestName;
    SplitURL(FinalURL, BaseURL, ManifestName);

    // 3. Handle --force bypass
    if (bForce)
    {
        LogCallback.ExecuteIfBound(false, FString::Printf(TEXT("Setting repo url (Forced) to: %s"), *FinalURL));
        SetRepoUrlCallback.ExecuteIfBound(BaseURL, ManifestName);
        return;
    }

    // 4. Download and validate manifest
    DownloadManifest(FinalURL, [LogCallback, SetRepoUrlCallback, BaseURL, ManifestName, FinalURL](bool bSuccess, const FString& ResponseContent)
    {
        if (!bSuccess)
        {
            LogCallback.ExecuteIfBound(true, FString::Printf(TEXT("FILE SIZE IS BELOW 2 BYTES OR DOWNLOAD FAILED! Check if URL is valid. (URL: %s)"), *FinalURL));
            return;
        }

        FLauncherManifest ParsedManifest;
        FString ParsingError;
        if (ParseManifest(ResponseContent, ParsedManifest, ParsingError))
        {
            LogCallback.ExecuteIfBound(false, FString::Printf(TEXT("Setting repo url to: %s"), *FinalURL));
            SetRepoUrlCallback.ExecuteIfBound(BaseURL, ManifestName);
        }
        else
        {
            // Safely truncate raw response preview in case it is too long
            FString RawPreview = ResponseContent.Left(150).Replace(TEXT("\n"), TEXT(" ")).Replace(TEXT("\r"), TEXT(""));

            // Log the parsing error and the raw content preview fetched from the server
            LogCallback.ExecuteIfBound(true, FString::Printf(
                TEXT("Repo manifest '%s' at URL '%s' has invalid syntax! Reason: %s | Raw Content Preview: [%s]"),
                                                             *ManifestName,
                                                             *FinalURL,
                                                             *ParsingError,
                                                             *RawPreview
            ));
        }
    });
}

void URepoCommandLibrary::DownloadManifest(const FString& TargetURL, TFunction<void(bool, const FString&)> Callback)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TargetURL);
    Request->SetVerb(TEXT("GET"));

    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
    {
        if (bWasSuccessful && ResponsePtr.IsValid())
        {
            const TArray<uint8>& RawData = ResponsePtr->GetContent();
            if (RawData.Num() >= 2 && ResponsePtr->GetResponseCode() == 200)
            {
                FString DecodedContent;
                // Automatically and safely decode bytes to string (handles UTF-8 and UTF-16 with BOM)
                FFileHelper::BufferToString(DecodedContent, RawData.GetData(), RawData.Num());

                // Strip any remaining BOM (0xFEFF) from the start if it wasn't cleaned by the helper
                if (DecodedContent.Len() > 0 && DecodedContent[0] == 0xFEFF)
                {
                    DecodedContent.RemoveAt(0);
                }

                Callback(true, DecodedContent);
                return;
            }
        }
        Callback(false, TEXT(""));
    });

    Request->ProcessRequest();
}

FString URepoCommandLibrary::CleanJsonString(const FString& RawString)
{
    int32 FirstBraceIndex = -1;
    int32 FirstBracketIndex = -1;

    const bool bHasBrace = RawString.FindChar('{', FirstBraceIndex);
    const bool bHasBracket = RawString.FindChar('[', FirstBracketIndex);

    int32 StartIndex = -1;
    if (bHasBrace && bHasBracket)
    {
        StartIndex = FMath::Min(FirstBraceIndex, FirstBracketIndex);
    }
    else if (bHasBrace)
    {
        StartIndex = FirstBraceIndex;
    }
    else if (bHasBracket)
    {
        StartIndex = FirstBracketIndex;
    }

    // If we found the first valid JSON character after some garbage/BOM, chop the string
    if (StartIndex > 0)
    {
        return RawString.RightChop(StartIndex);
    }

    return RawString;
}

bool URepoCommandLibrary::ParseManifest(const FString& JsonString, FLauncherManifest& OutManifest, FString& OutError)
{
    FString CleanedJson = CleanJsonString(JsonString);

    // Attempt 1: Try reading as a standard JSON Object {...}
    TSharedRef<TJsonReader<>> ObjectReader = TJsonReaderFactory<>::Create(CleanedJson);
    TSharedPtr<FJsonObject> JsonObject;
    if (FJsonSerializer::Deserialize(ObjectReader, JsonObject) && JsonObject.IsValid())
    {
        return ParseJsonObject(JsonObject.ToSharedRef(), OutManifest);
    }

    // Attempt 2: Try reading as a JSON Array [...] (requires a fresh reader)
    TSharedRef<TJsonReader<>> ArrayReader = TJsonReaderFactory<>::Create(CleanedJson);
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    if (FJsonSerializer::Deserialize(ArrayReader, JsonArray) && JsonArray.Num() > 0)
    {
        TSharedPtr<FJsonObject> FirstObject = JsonArray[0]->AsObject();
        if (FirstObject.IsValid())
        {
            return ParseJsonObject(FirstObject.ToSharedRef(), OutManifest);
        }
    }

    // If both parsing attempts failed, retrieve the error from the reader
    OutError = ArrayReader->GetErrorMessage();
    if (OutError.IsEmpty())
    {
        OutError = TEXT("JSON is neither a valid Object nor a non-empty Array.");
    }
    return false;
}

bool URepoCommandLibrary::ParseJsonObject(const TSharedRef<FJsonObject>& JsonObject, FLauncherManifest& OutManifest)
{
    JsonObject->TryGetStringField(TEXT("manifestName"), OutManifest.ManifestName);
    JsonObject->TryGetStringField(TEXT("manifestDesc"), OutManifest.ManifestDesc);
    JsonObject->TryGetNumberField(TEXT("manifestVersion"), OutManifest.ManifestVersion);
    JsonObject->TryGetStringField(TEXT("minSupportedVersion"), OutManifest.MinSupportedVersion);
    JsonObject->TryGetStringField(TEXT("latestVersion"), OutManifest.LatestVersion);

    const TSharedPtr<FJsonObject>* GamesURLsObj;
    if (JsonObject->TryGetObjectField(TEXT("gamesURLs"), GamesURLsObj))
    {
        for (const auto& KeyValue : (*GamesURLsObj)->Values)
        {
            OutManifest.GamesURLs.Add(KeyValue.Key, KeyValue.Value->AsString());
        }
    }

    const TSharedPtr<FJsonObject>* CustomMetaObj;
    if (JsonObject->TryGetObjectField(TEXT("customMeta"), CustomMetaObj))
    {
        for (const auto& KeyValue : (*CustomMetaObj)->Values)
        {
            OutManifest.CustomMeta.Add(KeyValue.Key, KeyValue.Value->AsString());
        }
    }

    return true;
}

void URepoCommandLibrary::SplitURL(const FString& FullURL, FString& OutBaseURL, FString& OutManifestName)
{
    int32 LastSlashIndex;
    if (FullURL.FindLastChar('/', LastSlashIndex))
    {
        OutBaseURL = FullURL.Left(LastSlashIndex + 1);
        OutManifestName = FullURL.RightChop(LastSlashIndex + 1);
    }
    else
    {
        OutBaseURL = FullURL;
        OutManifestName = TEXT("games.json");
    }
}