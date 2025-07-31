#include "JsonToDataTableHelper.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

static bool FillProperty(void* Ptr, FProperty* Prop, TSharedPtr<FJsonValue> JsonValue);

static bool FillStructRecursive(void* OutStruct, UStruct* StructType, TSharedPtr<FJsonObject> JsonObject)
{
    if (!OutStruct || !StructType || !JsonObject.IsValid())
        return false;

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        FProperty* Prop = *It;

        FString JsonFieldName = Prop->GetMetaData(TEXT("DisplayName"));
        if (JsonFieldName.IsEmpty())
            JsonFieldName = Prop->GetName();

        if (!JsonObject->HasField(JsonFieldName))
            continue;

        TSharedPtr<FJsonValue> JsonValue = JsonObject->TryGetField(JsonFieldName);
        void* PropAddr = Prop->ContainerPtrToValuePtr<void>(OutStruct);

        if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
        {
            StrProp->SetPropertyValue(PropAddr, JsonValue->AsString());
        }
        else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
        {
            NameProp->SetPropertyValue(PropAddr, FName(*JsonValue->AsString()));
        }
        else if (FTextProperty* TextProp = CastField<FTextProperty>(Prop))
        {
            TextProp->SetPropertyValue(PropAddr, FText::FromString(JsonValue->AsString()));
        }
        else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
        {
            IntProp->SetPropertyValue(PropAddr, (int32)JsonValue->AsNumber());
        }
        else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
        {
            FloatProp->SetPropertyValue(PropAddr, JsonValue->AsNumber());
        }
        else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
        {
            BoolProp->SetPropertyValue(PropAddr, JsonValue->AsBool());
        }
        else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
        {
            if (JsonValue->Type == EJson::Array)
            {
                FScriptArrayHelper Helper(ArrayProp, PropAddr);
                TArray<TSharedPtr<FJsonValue>> JsonArray = JsonValue->AsArray();

                for (int32 i = 0; i < JsonArray.Num(); ++i)
                {
                    int32 NewIdx = Helper.AddValue();
                    void* ElemPtr = Helper.GetRawPtr(NewIdx);
                    FillProperty(ElemPtr, ArrayProp->Inner, JsonArray[i]);
                }
            }
        }
        else if (FMapProperty* MapProp = CastField<FMapProperty>(Prop))
        {
            if (JsonValue->Type == EJson::Object)
            {
                FScriptMapHelper MapHelper(MapProp, PropAddr);
                TSharedPtr<FJsonObject> JsonMap = JsonValue->AsObject();
                for (const auto& Pair : JsonMap->Values)
                {
                    int32 NewIdx = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
                    void* KeyPtr = MapHelper.GetKeyPtr(NewIdx);
                    void* ValPtr = MapHelper.GetValuePtr(NewIdx);

                    if (FStrProperty* KeyStrProp = CastField<FStrProperty>(MapProp->KeyProp))
                    {
                        KeyStrProp->SetPropertyValue(KeyPtr, Pair.Key);
                    }
                    else if (FNameProperty* KeyNameProp = CastField<FNameProperty>(MapProp->KeyProp))
                    {
                        KeyNameProp->SetPropertyValue(KeyPtr, FName(*Pair.Key));
                    }
                    else
                    {
                        continue; // Unsupported key type
                    }

                    FillProperty(ValPtr, MapProp->ValueProp, Pair.Value);
                }
                MapHelper.Rehash();
            }
        }
        else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
        {
            if (JsonValue->Type == EJson::Object)
            {
                TSharedPtr<FJsonObject> SubObject = JsonValue->AsObject();
                FillStructRecursive(PropAddr, StructProp->Struct, SubObject);
            }
        }
    }

    return true;
}

static bool FillProperty(void* Ptr, FProperty* Prop, TSharedPtr<FJsonValue> JsonValue)
{
    if (!Prop || !JsonValue.IsValid()) return false;

    if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
    {
        StrProp->SetPropertyValue(Ptr, JsonValue->AsString());
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
    {
        NameProp->SetPropertyValue(Ptr, FName(*JsonValue->AsString()));
    }
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Prop))
    {
        TextProp->SetPropertyValue(Ptr, FText::FromString(JsonValue->AsString()));
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
    {
        IntProp->SetPropertyValue(Ptr, (int32)JsonValue->AsNumber());
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
    {
        FloatProp->SetPropertyValue(Ptr, JsonValue->AsNumber());
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
    {
        BoolProp->SetPropertyValue(Ptr, JsonValue->AsBool());
    }
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
    {
        return FillStructRecursive(Ptr, StructProp->Struct, JsonValue->AsObject());
    }

    return true;
}

UDataTable* UJsonToDataTableHelper::CreateComplexDataTableFromJson(const FString& JsonString, UScriptStruct* StructDefinition)
{
    if (!StructDefinition)
    {
        UE_LOG(LogTemp, Error, TEXT("StructDefinition is null"));
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, JsonArray))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON array."));
        return nullptr;
    }

    UDataTable* NewTable = NewObject<UDataTable>(UDataTable::StaticClass());
    NewTable->RowStruct = StructDefinition;

    for (int i = 0; i < JsonArray.Num(); i++)
    {
        TSharedPtr<FJsonObject> RowObject = JsonArray[i]->AsObject();
        if (!RowObject.IsValid())
            continue;

        uint8* RowData = (uint8*)FMemory::Malloc(StructDefinition->GetStructureSize());
        StructDefinition->InitializeStruct(RowData);

        if (!FillStructRecursive(RowData, StructDefinition, RowObject))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to fill row %d"), i);
            StructDefinition->DestroyStruct(RowData);
            FMemory::Free(RowData);
            continue;
        }

        FString RowName = FString::Printf(TEXT("Row_%d"), i);
        NewTable->AddRow(FName(*RowName), *(FTableRowBase*)RowData);
        StructDefinition->DestroyStruct(RowData);
        FMemory::Free(RowData);
    }

    return NewTable;
}
