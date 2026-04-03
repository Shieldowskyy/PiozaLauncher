// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Sockets.h"
#include "Networking.h"
#include "BootstrapperSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLaunchRequested, const FString&, GameID);

/**
 * Subsystem that listens for game launch requests from the bootstrapper via TCP.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UBootstrapperSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Called when a game launch is requested via the pioza:// protocol */
	UPROPERTY(BlueprintAssignable, Category = "Pioza|Bootstrapper")
	FOnGameLaunchRequested OnGameLaunchRequested;

private:
	/** The listener socket */
	FSocket* ListenerSocket = nullptr;

	/** Timer handle for checking connections */
	FTimerHandle SocketCheckTimerHandle;

	/** Port to listen on */
	const int32 Port = 55562;

	/** Check for new connections and data */
	void CheckSocketData();

	/** Handle a new connection */
	void HandleConnection(FSocket* ClientSocket);
};
