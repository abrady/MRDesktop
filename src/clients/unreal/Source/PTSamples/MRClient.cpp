// Fill out your copyright notice in the Description page of Project Settings.

#include "MRClient.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "HAL/RunnableThread.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "FrameUtils.h"
#include <vector>

MRClient::MRClient()
    : Socket(nullptr)
    , bIsConnected(false)
    , bShouldStop(false)
    , Thread(nullptr)
    , CurrentFrameWidth(0)
    , CurrentFrameHeight(0)
    , CurrentFrameDataSize(0)
    , RemoteTex(nullptr)
{
}

MRClient::~MRClient()
{
    Disconnect();
}

bool MRClient::ConnectToServer(const FString& ServerAddress, int32 Port)
{
    if (bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("MRClient: Already connected to server"));
        return true;
    }

    // Get socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("MRClient: Failed to get socket subsystem"));
        return false;
    }

    // Create socket
    Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("MRClient Socket"), false);
    if (!Socket)
    {
        UE_LOG(LogTemp, Error, TEXT("MRClient: Failed to create socket"));
        return false;
    }

    // Parse server address
    FIPv4Address IPv4Address;
    if (!FIPv4Address::Parse(ServerAddress, IPv4Address))
    {
        UE_LOG(LogTemp, Error, TEXT("MRClient: Invalid server address: %s"), *ServerAddress);
        Socket->Close();
        SocketSubsystem->DestroySocket(Socket);
        Socket = nullptr;
        return false;
    }

    // Create server endpoint
    TSharedRef<FInternetAddr> ServerAddr = SocketSubsystem->CreateInternetAddr();
    ServerAddr->SetIp(IPv4Address.Value);
    ServerAddr->SetPort(Port);

    // Connect to server
    UE_LOG(LogTemp, Log, TEXT("MRClient: Connecting to %s:%d"), *ServerAddress, Port);
    
    if (!Socket->Connect(*ServerAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("MRClient: Failed to connect to server"));
        Socket->Close();
        SocketSubsystem->DestroySocket(Socket);
        Socket = nullptr;
        return false;
    }

    bIsConnected = true;
    bShouldStop = false;

    // Start receiving thread
    Thread = FRunnableThread::Create(this, TEXT("MRClient Thread"));
    
    UE_LOG(LogTemp, Log, TEXT("MRClient: Successfully connected to server"));
    return true;
}

void MRClient::Disconnect()
{
    if (!bIsConnected)
        return;

    UE_LOG(LogTemp, Log, TEXT("MRClient: Disconnecting from server"));

    // Stop the thread
    bShouldStop = true;
    
    if (Thread)
    {
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }

    // Close socket
    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }

    bIsConnected = false;
}

bool MRClient::SendMouseMove(int32 DeltaX, int32 DeltaY, bool bAbsolute, int32 X, int32 Y)
{
    if (!bIsConnected || !Socket)
        return false;

    MouseMoveMessage msg;
    msg.header.type = MSG_MOUSE_MOVE;
    msg.header.size = sizeof(MouseMoveMessage);
    msg.deltaX = DeltaX;
    msg.deltaY = DeltaY;
    msg.absolute = bAbsolute ? 1 : 0;
    msg.x = X;
    msg.y = Y;

    int32 BytesSent = 0;
    return Socket->Send(reinterpret_cast<uint8*>(&msg), sizeof(MouseMoveMessage), BytesSent) 
           && BytesSent == sizeof(MouseMoveMessage);
}

bool MRClient::SendMouseClick(int32 Button, bool bPressed)
{
    if (!bIsConnected || !Socket)
        return false;

    MouseClickMessage msg;
    msg.header.type = MSG_MOUSE_CLICK;
    msg.header.size = sizeof(MouseClickMessage);
    msg.button = static_cast<MouseClickMessage::MouseButton>(Button);
    msg.pressed = bPressed ? 1 : 0;

    int32 BytesSent = 0;
    return Socket->Send(reinterpret_cast<uint8*>(&msg), sizeof(MouseClickMessage), BytesSent) 
           && BytesSent == sizeof(MouseClickMessage);
}

bool MRClient::SendMouseScroll(int32 DeltaX, int32 DeltaY)
{
    if (!bIsConnected || !Socket)
        return false;

    MouseScrollMessage msg;
    msg.header.type = MSG_MOUSE_SCROLL;
    msg.header.size = sizeof(MouseScrollMessage);
    msg.deltaX = DeltaX;
    msg.deltaY = DeltaY;

    int32 BytesSent = 0;
    return Socket->Send(reinterpret_cast<uint8*>(&msg), sizeof(MouseScrollMessage), BytesSent) 
           && BytesSent == sizeof(MouseScrollMessage);
}

bool MRClient::Init()
{
    return true;
}

uint32 MRClient::Run()
{
    UE_LOG(LogTemp, Log, TEXT("MRClient: Receive thread started"));

    while (!bShouldStop && bIsConnected)
    {
        if (!ReceiveFrameData())
        {
            // If we fail to receive data, wait a bit before trying again
            FPlatformProcess::Sleep(0.016f); // ~60fps
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MRClient: Receive thread stopping"));
    return 0;
}

void MRClient::Stop()
{
    bShouldStop = true;
}

void MRClient::Exit()
{
    bShouldStop = true;
}

bool MRClient::ReceiveFrameData()
{
    if (!Socket || !bIsConnected)
        return false;

    auto recvWrapper = [this](uint8* buffer, int len) -> int
    {
        int32 BytesReceived = 0;
        if (!Socket->Recv(buffer, len, BytesReceived))
        {
            ESocketErrors Err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
            if (Err == SE_EWOULDBLOCK)
                return 0;
            return -1;
        }
        return BytesReceived;
    };

    FrameMessage frameMsg;
    TArray<uint8> FrameData;
    std::vector<uint8> Temp;
    bool ok = ReadFrameGeneric(recvWrapper, frameMsg, Temp);
    if (!ok)
        return false;

    FrameData.Append(Temp.data(), Temp.size());

    CurrentFrameWidth = frameMsg.width;
    CurrentFrameHeight = frameMsg.height;
    CurrentFrameDataSize = frameMsg.dataSize;

    // Push the frame pixels to the dynamic texture
    PushFrame(FrameData.GetData(), CurrentFrameWidth, CurrentFrameHeight, CurrentFrameWidth * 4);

    // Call the frame received callback on the game thread
    AsyncTask(ENamedThreads::GameThread, [this, FrameData = MoveTemp(FrameData)]()
    {
        OnFrameReceived.ExecuteIfBound(FrameData);
    });

    return true;
}


void MRClient::CreateRemoteTexture(int32 Width, int32 Height)
{
    if (RemoteTex && RemoteTex->GetSizeX() == Width && RemoteTex->GetSizeY() == Height)
    {
        return;
    }

    RemoteTex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (RemoteTex)
    {
        RemoteTex->NeverStream = true;
        RemoteTex->SRGB = false;
        RemoteTex->UpdateResource();
    }
}

void MRClient::PushFrame(const uint8* Data, int32 W, int32 H, int32 PitchBytes)
{
    TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> Buffer = MakeShared<TArray<uint8>>();
    Buffer->Append(Data, W * H * 4);

    AsyncTask(ENamedThreads::GameThread, [this, Buffer, W, H, PitchBytes]()
    {
        CreateRemoteTexture(W, H);
        static FUpdateTextureRegion2D Region(0, 0, 0, 0, W, H);
        if (RemoteTex)
        {
            RemoteTex->UpdateTextureRegions(0, 1, &Region, PitchBytes, 4, Buffer->GetData(), nullptr);
        }
    });
}
