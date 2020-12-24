//+---------------------------------------------------------+
//| Project   : MedelDesign Climb System C++ UE 4.24		|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ClimbInterface.generated.h"

UINTERFACE(MinimalAPI)
class UClimbInterface : public UInterface
{
	GENERATED_BODY()
};

class CLIMBSYSTEM_API IClimbInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void CharacterCanGrab(bool bCharacterCanGrab);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void CharacterClimbLedge(bool bCharacterIsClimbing);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void MoveLeftRight(float Direction);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void JumpLeft(bool bJumpLeft);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void JumpRight(bool bJumpRight);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void JumpUp(bool bJumpUp);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void TurnBack(bool bTurnBack);
};
