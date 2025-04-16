#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NotificationFunctionLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UNotificationFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Sends a native OS notification! On Windows it is using Windows API. On Linux it uses notify-send shell command.
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	static void SendSystemNotification(const FString& Title, const FString& Message, const FString& AppName = "");
};
