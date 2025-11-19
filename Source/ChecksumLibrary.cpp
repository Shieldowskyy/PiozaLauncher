// Created by Shieldziak for DashoGames <3

#include "ChecksumLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/SecureHash.h"

bool UChecksumLibrary::CalculateFileChecksum(const FString& FilePath, EChecksumAlgorithm Algorithm, FString& OutChecksum)
{
    OutChecksum.Empty();

    // Check if file exists
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.FileExists(*FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *FilePath);
        return false;
    }

    switch (Algorithm)
    {
        case EChecksumAlgorithm::MD5:
        {
            FMD5 MD5;
            bool bSuccess = ReadFileInChunks(FilePath, [&MD5](const uint8* Data, int32 Size)
            {
                MD5.Update(Data, Size);
            });
            
            if (bSuccess)
            {
                uint8 Digest[16];
                MD5.Final(Digest);
                OutChecksum = BytesToHexString(Digest, 16);
            }
            return bSuccess;
        }

        case EChecksumAlgorithm::SHA1:
        {
            FSHA1 SHA1;
            bool bSuccess = ReadFileInChunks(FilePath, [&SHA1](const uint8* Data, int32 Size)
            {
                SHA1.Update(Data, Size);
            });
            
            if (bSuccess)
            {
                uint8 Digest[20];
                SHA1.Final();
                SHA1.GetHash(Digest);
                OutChecksum = BytesToHexString(Digest, 20);
            }
            return bSuccess;
        }

        case EChecksumAlgorithm::CRC32:
        {
            uint32 CRC = 0;
            bool bSuccess = ReadFileInChunks(FilePath, [&CRC](const uint8* Data, int32 Size)
            {
                CRC = FCrc::MemCrc32(Data, Size, CRC);
            });
            
            if (bSuccess)
            {
                OutChecksum = FString::Printf(TEXT("%08x"), CRC);
            }
            return bSuccess;
        }
    }

    return false;
}

bool UChecksumLibrary::VerifyFileChecksum(const FString& FilePath, const FString& ExpectedChecksum, EChecksumAlgorithm Algorithm)
{
    FString ActualChecksum;
    if (!CalculateFileChecksum(FilePath, Algorithm, ActualChecksum))
    {
        return false;
    }

    // Case-insensitive comparison
    return ActualChecksum.Equals(ExpectedChecksum, ESearchCase::IgnoreCase);
}

bool UChecksumLibrary::LoadChecksumsFromFile(const FString& ChecksumFilePath, TMap<FString, FString>& OutChecksums)
{
    OutChecksums.Empty();

    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *ChecksumFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load checksum file: %s"), *ChecksumFilePath);
        return false;
    }

    for (const FString& Line : Lines)
    {
        FString TrimmedLine = Line.TrimStartAndEnd();
        if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
        {
            continue; // Skip empty lines and comments
        }

        // Expected format: "checksum filepath" or "checksum  filepath"
        FString Checksum, FilePath;
        if (TrimmedLine.Split(TEXT(" "), &Checksum, &FilePath, ESearchCase::IgnoreCase, ESearchDir::FromStart))
        {
            FilePath = FilePath.TrimStartAndEnd();
            Checksum = Checksum.TrimStartAndEnd();
            
            if (!Checksum.IsEmpty() && !FilePath.IsEmpty())
            {
                OutChecksums.Add(FilePath, Checksum);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Loaded %d checksums from file"), OutChecksums.Num());
    return OutChecksums.Num() > 0;
}

FString UChecksumLibrary::GetAlgorithmName(EChecksumAlgorithm Algorithm)
{
    switch (Algorithm)
    {
        case EChecksumAlgorithm::MD5:    return TEXT("MD5");
        case EChecksumAlgorithm::SHA1:   return TEXT("SHA-1");
        case EChecksumAlgorithm::CRC32:  return TEXT("CRC-32");
        default: return TEXT("Unknown");
    }
}

bool UChecksumLibrary::ReadFileInChunks(const FString& FilePath, TFunction<void(const uint8*, int32)> ProcessChunk)
{
    // Read file in 1MB chunks to avoid loading huge files into memory
    const int32 ChunkSize = 1024 * 1024;
    
    IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*FilePath);
    if (!FileHandle)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open file: %s"), *FilePath);
        return false;
    }

    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(ChunkSize);

    int64 FileSize = FileHandle->Size();
    int64 BytesRead = 0;

    while (BytesRead < FileSize)
    {
        int64 BytesToRead = FMath::Min<int64>(ChunkSize, FileSize - BytesRead);
        if (!FileHandle->Read(Buffer.GetData(), BytesToRead))
        {
            delete FileHandle;
            return false;
        }

        ProcessChunk(Buffer.GetData(), BytesToRead);
        BytesRead += BytesToRead;
    }

    delete FileHandle;
    return true;
}

FString UChecksumLibrary::BytesToHexString(const uint8* Bytes, int32 Length)
{
    FString Result;
    Result.Reserve(Length * 2);

    for (int32 i = 0; i < Length; ++i)
    {
        Result += FString::Printf(TEXT("%02x"), Bytes[i]);
    }

    return Result;
}