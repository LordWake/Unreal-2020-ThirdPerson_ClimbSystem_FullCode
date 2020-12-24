// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ClimbSystemGameMode.h"
#include "ClimbSystemCharacter.h"
#include "UObject/ConstructorHelpers.h"

AClimbSystemGameMode::AClimbSystemGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
