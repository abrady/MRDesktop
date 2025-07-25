#include "MRClientActor.h"
#include "Engine/Engine.h"

AMRClientActor::AMRClientActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMRClientActor::BeginPlay()
{
    Super::BeginPlay();

    Client = MakeUnique<MRClient>();
    Client->OnFrameReceived.BindUObject(this, &AMRClientActor::HandleFrame);
    Client->ConnectToServer();
}

void AMRClientActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Client.IsValid())
    {
        Client->Disconnect();
        Client.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void AMRClientActor::HandleFrame(const TArray<uint8>& Data)
{
    UE_LOG(LogTemp, Log, TEXT("Received frame: %d bytes"), Data.Num());
}

