// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "JsonObjectConverter.h"
#include "JsonParserLib.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UJsonParserLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Parses a JSON string into a struct of the specified type.
	 * @param JsonString - JSON to parse.
	 * @param StructType - Pointer to the struct type to parse into.
	 * @param OutStruct - Output instantiated struct with parsed data.
	 * @return True if parsing succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category="JSON", meta=(DeterminesOutputType="StructType"))
	static bool JsonToStruct(const FString& JsonString, UScriptStruct* StructType, FInstancedStruct& OutStruct);
};
