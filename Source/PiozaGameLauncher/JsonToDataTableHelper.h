#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "JsonToDataTableHelper.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UJsonToDataTableHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "JSON|DataTable", meta = (DisplayName = "Parse Complex JSON to DataTable"))
    static UDataTable* CreateComplexDataTableFromJson(const FString& JsonString, UScriptStruct* StructDefinition, bool& bSuccess);
};
