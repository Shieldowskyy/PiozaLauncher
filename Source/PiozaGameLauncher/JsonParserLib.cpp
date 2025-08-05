#include "JsonParserLib.h"

bool UJsonParserLib::JsonToStruct(const FString& JsonString, UScriptStruct* StructType, FInstancedStruct& OutStruct)
{
	OutStruct.Reset();

	if (!StructType)
	{
		UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: StructType is null"));
		return false;
	}

	if (JsonString.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: JsonString is empty"));
		return false;
	}

	TSharedPtr<FJsonValue> JsonValue;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: Failed to parse JSON"));
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;

	// Jeśli JSON jest tablicą, to bierzemy pierwszy element jako obiekt
	if (JsonValue->Type == EJson::Array)
	{
		const TArray<TSharedPtr<FJsonValue>>* JsonArray;
		if (!JsonValue->TryGetArray(JsonArray) || JsonArray->Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: JSON array is empty or invalid"));
			return false;
		}

		JsonObject = (*JsonArray)[0]->AsObject();

		if (!JsonObject.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: First element of JSON array is not an object"));
			return false;
		}
	}
	else if (JsonValue->Type == EJson::Object)
	{
		JsonObject = JsonValue->AsObject();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: JSON root is not an object or array"));
		return false;
	}

	OutStruct.InitializeAs(StructType, nullptr);

	bool bSuccess = FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructType, OutStruct.GetMutableMemory(), 0, 0);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("JsonToStruct: JsonObjectToUStruct failed"));
		OutStruct.Reset();
	}

	return bSuccess;
}
