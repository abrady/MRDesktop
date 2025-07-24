// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/Thread.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Templates/SharedPointer.h"

// Forward declarations
struct FrameMessage;

DECLARE_DELEGATE_OneParam(FOnFrameReceived, const TArray<uint8>&);

/**
 * MRClient - Handles connection to MRDesktop server and frame reception
 */
class UNREALCLIENT_API MRClient : public FRunnable
{
public:
	MRClient();
	virtual ~MRClient();

	// Connection management
	bool ConnectToServer(const FString& ServerAddress = TEXT("127.0.0.1"), int32 Port = 8080);
	void Disconnect();
	bool IsConnected() const { return bIsConnected; }

	// Input sending
	bool SendMouseMove(int32 DeltaX, int32 DeltaY, bool bAbsolute = false, int32 X = 0, int32 Y = 0);
	bool SendMouseClick(int32 Button, bool bPressed);
	bool SendMouseScroll(int32 DeltaX, int32 DeltaY);

	// Frame data callback
	FOnFrameReceived OnFrameReceived;

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

private:
	// Socket management
	class FSocket* Socket;
	FThreadSafeBool bIsConnected;
	FThreadSafeBool bShouldStop;
	
	// Threading
	FRunnableThread* Thread;
	
	// Frame reception
	bool ReceiveFrameData();
	bool ReceiveExactBytes(uint8* Buffer, int32 BytesToReceive);
	
	// Current frame info
	uint32 CurrentFrameWidth;
	uint32 CurrentFrameHeight;
	uint32 CurrentFrameDataSize;
};
