#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MRClient.h"
#include "MRClientActor.generated.h"

/**
 * Simple actor that owns an MRClient and logs received frames
 */
UCLASS()
class PTSAMPLES_API AMRClientActor : public AActor
{
    GENERATED_BODY()

public:
    AMRClientActor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void HandleFrame(const TArray<uint8>& Data);

private:
    TUniquePtr<MRClient> Client;
};

