#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GetAndSetRHI.generated.h"

/**
 * 
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UGetAndSetRHI : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	//Returns current RHI name as a string.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Engine")
	static FString GetCurrentRhiName();
};

