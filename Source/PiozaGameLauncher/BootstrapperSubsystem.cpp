// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "BootstrapperSubsystem.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Common/TcpListener.h"
#include "Containers/UnrealString.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

void UBootstrapperSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FIPv4Endpoint Endpoint(FIPv4Address::Any, Port);
	ListenerSocket = FTcpSocketBuilder(TEXT("BootstrapperListener"))
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);

	if (ListenerSocket)
	{
		UE_LOG(LogTemp, Log, TEXT("BootstrapperSubsystem: Listening on port %d"), Port);
		
		// Set up polling for new connections
		GetWorld()->GetTimerManager().SetTimer(SocketCheckTimerHandle, this, &UBootstrapperSubsystem::CheckSocketData, 0.5f, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BootstrapperSubsystem: Failed to create listener socket on port %d"), Port);
	}

	// Check command line for initial game ID
	FString CommandLineGameID;
	if (FParse::Value(FCommandLine::Get(), TEXT("start-game="), CommandLineGameID))
	{
		PendingGameID = CommandLineGameID;
		UE_LOG(LogTemp, Log, TEXT("BootstrapperSubsystem: Found start-game ID in command line: %s"), *PendingGameID);
	}
}

void UBootstrapperSubsystem::Deinitialize()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SocketCheckTimerHandle);
	}

	if (ListenerSocket)
	{
		ListenerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
	}

	Super::Deinitialize();
}

void UBootstrapperSubsystem::CheckSocketData()
{
	if (!ListenerSocket) return;

	bool bHasPendingConnection;
	if (ListenerSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
	{
		TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		FSocket* ClientSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("BootstrapperClient"));

		if (ClientSocket)
		{
			HandleConnection(ClientSocket);
		}
	}
}

void UBootstrapperSubsystem::HandleConnection(FSocket* ClientSocket)
{
	TArray<uint8> ReceivedData;
	uint32 Size;
	
	// Wait a bit for data to arrive if it's not immediate
	FPlatformProcess::Sleep(0.05f);

	if (ClientSocket->HasPendingData(Size))
	{
		ReceivedData.Init(0, Size + 1);
		int32 Read = 0;
		ClientSocket->Recv(ReceivedData.GetData(), Size, Read);

		if (Read > 0)
		{
			FString GameID = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			GameID = GameID.TrimStartAndEnd();
			
			UE_LOG(LogTemp, Log, TEXT("BootstrapperSubsystem: Received game launch request for: %s"), *GameID);
			
			PendingGameID = GameID;
			OnGameLaunchRequested.Broadcast(GameID);
		}
	}

	ClientSocket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
}

FString UBootstrapperSubsystem::ConsumePendingGameID()
{
	FString Result = PendingGameID;
	PendingGameID = TEXT("");
	return Result;
}
