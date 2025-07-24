// Fill out your copyright notice in the Description page of Project Settings.

#include "MRClient.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "HAL/RunnableThread.h"
#include "Engine/Engine.h"

// Include protocol definitions
#pragma pack(push, 1)

enum MessageType : uint32 {
    MSG_FRAME_DATA = 1,
    MSG_MOUSE_MOVE = 2,
    MSG_MOUSE_CLICK = 3,
    MSG_MOUSE_SCROLL = 4
};

struct MessageHeader {
    MessageType type;
    uint32 size;
};

struct FrameMessage {
    MessageHeader header;
    uint32 width;
    uint32 height;
    uint32 dataSize;
};

struct MouseMoveMessage {
    MessageHeader header;
    int32 deltaX;
    int32 deltaY;
    int32 absolute;
    int32 x;
    int32 y;
};

struct MouseClickMessage {
    MessageHeader header;
    enum MouseButton : uint32 {
        LEFT_BUTTON = 1,
        RIGHT_BUTTON = 2,
        MIDDLE_BUTTON = 4
    } button;
    int32 pressed;
};

struct MouseScrollMessage {
    MessageHeader header;
    int32 deltaX;
    int32 deltaY;
};

#pragma pack(pop)

MRClient::MRClient()
    : Socket(nullptr)
    , bIsConnected(false)
    , bShouldStop(false)
    , Thread(nullptr)
    , CurrentFrameWidth(0)
    , CurrentFrameHeight(0)
    , CurrentFrameDataSize(0)
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

    // First, receive the frame message header
    FrameMessage frameMsg;
    if (!ReceiveExactBytes(reinterpret_cast<uint8*>(&frameMsg), sizeof(FrameMessage)))
    {
        return false;
    }

    // Validate message type
    if (frameMsg.header.type != MSG_FRAME_DATA)
    {
        UE_LOG(LogTemp, Warning, TEXT("MRClient: Received unexpected message type: %d"), frameMsg.header.type);
        return false;
    }

    // Update current frame info
    CurrentFrameWidth = frameMsg.width;
    CurrentFrameHeight = frameMsg.height;
    CurrentFrameDataSize = frameMsg.dataSize;

    // Allocate buffer for frame data
    TArray<uint8> FrameData;
    FrameData.SetNumUninitialized(CurrentFrameDataSize);

    // Receive the actual frame pixel data
    if (!ReceiveExactBytes(FrameData.GetData(), CurrentFrameDataSize))
    {
        UE_LOG(LogTemp, Error, TEXT("MRClient: Failed to receive frame pixel data"));
        return false;
    }

    // Call the frame received callback on the game thread
    AsyncTask(ENamedThreads::GameThread, [this, FrameData = MoveTemp(FrameData)]()
    {
        if (OnFrameReceived.IsBound())
        {
            OnFrameReceived.Execute(FrameData);
        }
    });

    return true;
}

bool MRClient::ReceiveExactBytes(uint8* Buffer, int32 BytesToReceive)
{
    if (!Socket || !Buffer || BytesToReceive <= 0)
        return false;

    int32 TotalBytesReceived = 0;
    
    while (TotalBytesReceived < BytesToReceive && !bShouldStop)
    {
        int32 BytesReceived = 0;
        
        if (!Socket->Recv(Buffer + TotalBytesReceived, 
                         BytesToReceive - TotalBytesReceived, 
                         BytesReceived))
        {
            // Check if it's a would-block error (non-blocking socket)
            ESocketErrors SocketError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
            if (SocketError == SE_EWOULDBLOCK)
            {
                // No data available right now, wait a bit
                FPlatformProcess::Sleep(0.001f);
                continue;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("MRClient: Socket receive error: %d"), (int32)SocketError);
                return false;
            }
        }

        if (BytesReceived == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("MRClient: Connection closed by server"));
            bIsConnected = false;
            return false;
        }

        TotalBytesReceived += BytesReceived;
    }

    return TotalBytesReceived == BytesToReceive;
}
