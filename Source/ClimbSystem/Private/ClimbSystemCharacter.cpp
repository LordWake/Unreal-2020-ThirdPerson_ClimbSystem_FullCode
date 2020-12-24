//+---------------------------------------------------------+
//| Project   : MedelDesign Climb System C++ UE 4.24		|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#include "ClimbSystemCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"


AClimbSystemCharacter::AClimbSystemCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	BaseTurnRate	= 45.f;
	BaseLookUpRate	= 45.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw	= false;
	bUseControllerRotationRoll	= false;

	// Configure Character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate	= FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity	= 600.f;
	GetCharacterMovement()->AirControl		= 0.2f;

	//Spring Arm
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength			= 300.0f; 
	CameraBoom->bUsePawnControlRotation = true; 

	//Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; 

	//Right Arrow Component. Used when the player is hanging and moving.
	RightArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RightArrow"));
	RightArrow->SetupAttachment(GetCapsuleComponent());
	RightArrow->SetRelativeLocation(FVector(40.0f, 70.0f, 40.0f));

	//Left Arrow Component. Used when the player is hanging and moving.
	LeftArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftArrow"));
	LeftArrow->SetupAttachment(GetCapsuleComponent());
	LeftArrow->SetRelativeLocation(FVector(40.0f, -70.0f, 40.0f));

	//RightLedge Arrow Component. Used when the player is trying to jump from wall to another.
	RightLedge = CreateDefaultSubobject<UArrowComponent>(TEXT("RightLedge"));
	RightLedge->SetupAttachment(GetCapsuleComponent());
	RightLedge->SetRelativeLocation(FVector(50.0f, 150.0f, 40.0f));

	//LeftLedge Arrow Component. Used when the player is trying to jump from wall to another.
	LeftLedge = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftLedge"));
	LeftLedge->SetupAttachment(GetCapsuleComponent());
	LeftLedge->SetRelativeLocation(FVector(50.0f, -150.0f, 40.0f));

	//Up Arrow Component. Used when the player jumps up.
	UpArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("UpArrow"));
	UpArrow->SetupAttachment(GetCapsuleComponent());
	UpArrow->SetRelativeLocation(FVector(65.0f, 0.0f, 290.0f));
}

void AClimbSystemCharacter::BeginPlay()
{
	Super::BeginPlay();
	MyCharacterMesh = FindComponentByClass<USkeletalMeshComponent>();
}

void AClimbSystemCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ForwardTracer();
	HeightTracer();
	JumpUpTracer();
	MoveSides();
	CheckForJumpOnTheSides();
}

void AClimbSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAction("Jump",		IE_Pressed, this, &AClimbSystemCharacter::CheckForJump);
	PlayerInputComponent->BindAction("ExitClimb",	IE_Pressed, this, &AClimbSystemCharacter::CheckForTurnBackOrExit);
	PlayerInputComponent->BindAction("LeftCorner",	IE_Pressed, this, &AClimbSystemCharacter::TurnToWallLeftCorner);
	PlayerInputComponent->BindAction("RightCorner", IE_Pressed, this, &AClimbSystemCharacter::TurnToWallRightCorner);	
	PlayerInputComponent->BindAction("Forward",		IE_Pressed, this, &AClimbSystemCharacter::CharacterTurnForward);

	PlayerInputComponent->BindAxis("MoveForward",	this, &AClimbSystemCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight",		this, &AClimbSystemCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn",			this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate",		this, &AClimbSystemCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp",		this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate",	this, &AClimbSystemCharacter::LookUpAtRate);
}

#pragma region Character Movement Inputs

void AClimbSystemCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AClimbSystemCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AClimbSystemCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && !bCharacterIsHanging)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AClimbSystemCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) && !bCharacterIsHanging)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AClimbSystemCharacter::CheckForJump()
{
	if (bCharacterIsHanging)
	{
		if (!bTurnedBack)
		{
			if (bCanJumpRight)
			{
				if (GetInputAxisValue("MoveRight") > 0)
					JumpRightLeftLedge(true);
				else
					ClimbLedge();
			}
			
			else
			{
				if (bCanJumpLeft)
				{
					if (GetInputAxisValue("MoveRight") < 0)
						JumpRightLeftLedge(false);
					else
						ClimbLedge();
				}
				else
				{
					if (bCanJumpUp)
						JumpUpLedge();
					else
						ClimbLedge();
				}
			}
		}
		
		else
		{
			if (!bIsJumping)
				JumpBack();
		}
	}
	
	else
		Jump();
}

void AClimbSystemCharacter::CheckForTurnBackOrExit()
{
	if (bCharacterIsHanging)
	{
		if (bTurnedBack)
		{
			bTurnedBack = false;
			
			if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
				IClimbInterface::Execute_TurnBack(MyCharacterMesh->GetAnimInstance(), false);

			ExitClimb();
		}
		else
			CharacterTurnBack();
	}
}

#pragma endregion

#pragma region Climb Wall

void AClimbSystemCharacter::ForwardTracer()
{
	FHitResult HitResult;
	
	const FVector TempForwardVector =	UKismetMathLibrary::GetForwardVector(GetActorRotation());	
	const FVector EndVector			=	GetActorLocation() + FVector(TempForwardVector.X * 150.0f, 
										TempForwardVector.Y * 150.0f, TempForwardVector.Z);

	const FCollisionShape MySphere	= FCollisionShape::MakeSphere(20.0f);
	
	const bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, GetActorLocation(), 
						EndVector, FQuat::Identity, ECC_GameTraceChannel1, MySphere);
	
	if (bOnHit)
	{
		WallLocation	= HitResult.Location;
		WallNormal		= HitResult.Normal;
	}
}

void AClimbSystemCharacter::HeightTracer()
{
	FHitResult HitResult;

	const FVector StartVector	=	FVector(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 500.0f) +
									FVector(UKismetMathLibrary::GetForwardVector(GetActorRotation()) * 70.0f);
	const FVector EndVector		=	FVector(StartVector.X, StartVector.Y, StartVector.Z - 500.0f);

	const FCollisionShape MySphere = FCollisionShape::MakeSphere(20.0f);
	
	const bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, StartVector, 
						EndVector, FQuat::Identity, ECC_GameTraceChannel1, MySphere);

	if (bOnHit)
	{
		WallHeightLocation = HitResult.Location;

		const FVector PelvisSocketLocation	= MyCharacterMesh->GetSocketLocation("PelvisSocket");	 
		const float rangeValue				= PelvisSocketLocation.Z - WallHeightLocation.Z;
		const bool bInRange					= UKismetMathLibrary::InRange_FloatFloat(rangeValue, -50.0f, 0.0f, true, true);

		if (bInRange)
		{
			if (!bIsClimbingLedge)
			{
				if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
					IClimbInterface::Execute_CharacterCanGrab(MyCharacterMesh->GetAnimInstance(), true);

					GetCharacterMovement()->SetMovementMode(MOVE_Flying);
					bCharacterIsHanging = true;
				
					GrabLedge();
			}
		}
	}
}

void AClimbSystemCharacter::ClimbLedge()
{
	if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
		IClimbInterface::Execute_CharacterClimbLedge(MyCharacterMesh->GetAnimInstance(), true);

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	bIsClimbingLedge	= true;
	bCharacterIsHanging = false;
}

void AClimbSystemCharacter::ExitClimb()
{
	if (bCharacterIsHanging)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
			IClimbInterface::Execute_CharacterCanGrab(MyCharacterMesh->GetAnimInstance(), false);

		bCharacterIsHanging = false;
	}
}

void AClimbSystemCharacter::LedgeMovementFinished()
{
	GetCharacterMovement()->StopMovementImmediately();
}

void AClimbSystemCharacter::GrabLedge()
{
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget		= this;
	LatentInfo.ExecutionFunction	= FName("LedgeMovementFinished");
	LatentInfo.Linkage				= 0;
	LatentInfo.UUID					= GiveMeAnUUIDNumber();

	const FVector WallNormalMultiplied = WallNormal * FVector(22.0f, 22.0f, 22.0f);
	const FVector TargetRelativeLocation(WallNormalMultiplied.X + WallLocation.X, WallNormalMultiplied.Y + WallLocation.Y, WallHeightLocation.Z - 120.0f);

	const FRotator TempRotation				= UKismetMathLibrary::Conv_VectorToRotator(WallNormal);
	const FRotator TargetRelativoRotation	= UKismetMathLibrary::MakeRotator (TempRotation.Roll, TempRotation.Pitch,TempRotation.Yaw - 180.0f);

	UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), TargetRelativeLocation,
	TargetRelativoRotation, false, false, 0.13f, false, EMoveComponentAction::Move, LatentInfo);
}

void AClimbSystemCharacter::CharacterClimbLedge_Implementation(bool bCharacterIsClimbing)
{
	bIsClimbingLedge = bCharacterIsClimbing;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

#pragma endregion

#pragma region Move On The Sides While Hanging

void AClimbSystemCharacter::MoveSides()
{
	if (bCharacterIsHanging)
	{
		RightLeftTracer(false);
		RightLeftTracer(true);
	}

	MoveCharacterOnTheSides(bCharacterIsHanging);
}

void AClimbSystemCharacter::RightLeftTracer(const bool& bRight)
{
	if (bRight)
	{
		FHitResult HitResult;
		const FCollisionShape MyCapsule = FCollisionShape::MakeCapsule(20.0f, 60.0f);

		const bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, RightArrow->GetComponentLocation(),
							RightArrow->GetComponentLocation(), FQuat::Identity, ECC_GameTraceChannel1, MyCapsule);

		bCanMoveRight = bOnHit ? true : false;
	}
	
	else
	{
		FHitResult HitResult;
		const FCollisionShape MyCapsule = FCollisionShape::MakeCapsule(20.0f, 60.0f);

		const bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, LeftArrow->GetComponentLocation(),
							LeftArrow->GetComponentLocation(), FQuat::Identity, ECC_GameTraceChannel1, MyCapsule);

		bCanMoveLeft = bOnHit ? true : false;
	}
}

void AClimbSystemCharacter::MoveCharacterOnTheSides(const bool& bMoving)
{
	if(bTurnedBack) return;

	if (bMoving)
	{
		IClimbInterface::Execute_MoveLeftRight(MyCharacterMesh->GetAnimInstance(), GetInputAxisValue("MoveRight"));
		MoveInLedge();
	}
	
	else
	{
		bMovingRight	= false;
		bMovingLeft		= false;
	}
}

void AClimbSystemCharacter::MoveInLedge()
{
	if (bCanMoveRight && GetInputAxisValue("MoveRight") > 0)
	{
		const FVector TargetLocation	=	GetActorLocation() + (UKismetMathLibrary::GetRightVector(GetActorRotation()) * 20.0f);	
		const FVector InterpVector		=	UKismetMathLibrary::VInterpTo(GetActorLocation(), TargetLocation, 
											UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), moveSidesSpeed);
		
		SetActorLocation(InterpVector);
		
		bMovingRight	= true;
		bMovingLeft		= false;
	}

	else if (bCanMoveLeft && GetInputAxisValue("MoveRight") < 0)
	{
		const FVector TargetLocation	=	GetActorLocation() + (UKismetMathLibrary::GetRightVector(GetActorRotation()) * -20.0f);
		const FVector InterpVector		=	UKismetMathLibrary::VInterpTo(GetActorLocation(), TargetLocation,
											UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), moveSidesSpeed);

		SetActorLocation(InterpVector);

		bMovingRight	= false;
		bMovingLeft		= true;
	}

	else if (GetInputAxisValue("MoveRight") == 0)
	{
		bMovingRight	= false;
		bMovingLeft		= false;
	}
}

#pragma endregion

#pragma region Jump To the Side Walls

void AClimbSystemCharacter::CheckForJumpOnTheSides()
{
	if (bCharacterIsHanging)
	{
		if (bCanMoveLeft)
		{
			bCanJumpLeft = false;
			bCanTurnLeft = false;
		}
		else
		{
			JumpRightLeftTracer(false);

			if (bCanJumpLeft)
				bCanTurnLeft = false;
			else
				TurnCornerRightLeftTracer(false);
		}

		if (bCanMoveRight)
		{
			bCanJumpRight = false;
			bCanTurnRight = false;
		}
		else
		{
			JumpRightLeftTracer(true);

			if (bCanJumpRight)
				bCanTurnRight = false;
			else
				TurnCornerRightLeftTracer(true);
		}
	}

}

void AClimbSystemCharacter::JumpRightLeftTracer(const bool& bRight)
{
	if (bRight)
	{
		FHitResult HitResult;
		const FCollisionShape MyCapsule = FCollisionShape::MakeCapsule(25.0f, 60.0f);

		const bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, RightLedge->GetComponentLocation(),
							RightLedge->GetComponentLocation(), FQuat::Identity, ECC_GameTraceChannel1, MyCapsule);

		if (bOnHit)
			bCanJumpRight = bCanMoveRight ? false : true;		
		else
			bCanJumpRight = false;
	}
	
	else
	{
		FHitResult HitResult;
		const FCollisionShape MyCapsule = FCollisionShape::MakeCapsule(25.0f, 60.0f);

		bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, LeftLedge->GetComponentLocation(),
						LeftLedge->GetComponentLocation(), FQuat::Identity, ECC_GameTraceChannel1, MyCapsule);

		if (bOnHit)
			bCanJumpLeft = bCanMoveLeft ? false : true;
		else
			bCanJumpLeft = false;
	}
}

void AClimbSystemCharacter::JumpRight_Implementation(bool bJumpRight)
{
	bIsJumping = bJumpRight;
	GetCharacterMovement()->StopMovementImmediately();
	bIsJumping = false;
}

void AClimbSystemCharacter::JumpLeft_Implementation(bool bJumpLeft)
{
	bIsJumping = bJumpLeft;
	GetCharacterMovement()->StopMovementImmediately();
	bIsJumping = false;
}

void AClimbSystemCharacter::JumpRightLeftLedge(const bool& bRight)
{
	if (bRight)
	{
		if (GetInputAxisValue("MoveRight") > 0 && !bIsJumping)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);

			if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
				IClimbInterface::Execute_JumpRight(MyCharacterMesh->GetAnimInstance(), true);

			bIsJumping				= true;
			bCharacterIsHanging		= true;

			FLatentActionInfo LatentInfo;
			LatentInfo.CallbackTarget		= this;
			LatentInfo.ExecutionFunction	= FName("GrabLedge");
			LatentInfo.Linkage				= 0;
			LatentInfo.UUID					= GiveMeAnUUIDNumber();

			UKismetSystemLibrary::Delay(GetWorld(), 0.8f, LatentInfo);
		}
	}
	
	else
	{
		if (GetInputAxisValue("MoveRight") < 0 && !bIsJumping)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);

			if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
				IClimbInterface::Execute_JumpLeft(MyCharacterMesh->GetAnimInstance(), true);

			bIsJumping				= true;
			bCharacterIsHanging		= true;

			FLatentActionInfo LatentInfo;
			LatentInfo.CallbackTarget		= this;
			LatentInfo.ExecutionFunction	= FName("GrabLedge");
			LatentInfo.Linkage				= 0;
			LatentInfo.UUID					= GiveMeAnUUIDNumber();

			UKismetSystemLibrary::Delay(GetWorld(), 0.8f, LatentInfo);
		}
	}
}

#pragma endregion

#pragma region Turn Around Corner

void AClimbSystemCharacter::TurnCornerRightLeftTracer(const bool& bRight)
{
	if (bRight)
	{
		GetCharacterMovement()->StopMovementImmediately();

		FHitResult HitResult;

		const FVector StartVector	= UKismetMathLibrary::MakeVector(RightArrow->GetComponentLocation().X,
									  RightArrow->GetComponentLocation().Y, RightArrow->GetComponentLocation().Z + 60.0f);
		const FVector EndVector		= StartVector + (RightArrow->GetForwardVector() * 70.0f);

		const FCollisionShape MySphere = FCollisionShape::MakeSphere(20.0f);

		const bool bOnHit = GetWorld()->SweepSingleByChannel(HitResult,StartVector, EndVector, 
							FQuat::Identity, ECC_GameTraceChannel1, MySphere);

		bCanTurnRight = bOnHit ? false : true;
	}
	
	else
	{
		GetCharacterMovement()->StopMovementImmediately();

		FHitResult HitResult;

		const FVector StartVector	= UKismetMathLibrary::MakeVector(LeftArrow->GetComponentLocation().X,
									  LeftArrow->GetComponentLocation().Y, LeftArrow->GetComponentLocation().Z + 60.0f);
		const FVector EndVector		= StartVector + (LeftArrow->GetForwardVector() * 70.0f);

		const FCollisionShape MySphere = FCollisionShape::MakeSphere(20.0f);

		const bool bOnHit = GetWorld()->SweepSingleByChannel(HitResult, StartVector, EndVector, 
							FQuat::Identity,ECC_GameTraceChannel1, MySphere);

		bCanTurnLeft = bOnHit ? false : true;
	}
}

void AClimbSystemCharacter::TurnToWallLeftCorner()
{
	if (bCharacterIsHanging)
	{
		if (!bCanJumpLeft && bCanTurnLeft)
		{
			DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			
			PlayAnimMontage(CornerLeftMontage, 1.0f, NAME_None);

			FLatentActionInfo LatentInfo;
			LatentInfo.CallbackTarget			= this;
			LatentInfo.ExecutionFunction		= FName("GrabLedge");
			LatentInfo.Linkage					= 0;
			LatentInfo.UUID						= GiveMeAnUUIDNumber();

			FLatentActionInfo LatentInfoInputs;
			LatentInfoInputs.CallbackTarget		= this;
			LatentInfoInputs.ExecutionFunction	= FName("EnablePlayerInputs");
			LatentInfoInputs.Linkage			= 0;
			LatentInfoInputs.UUID				= GiveMeAnUUIDNumber();

			UKismetSystemLibrary::Delay(GetWorld(), 0.8f, LatentInfo);
			UKismetSystemLibrary::Delay(GetWorld(), 1.5f, LatentInfoInputs);
		}
	}
}

void AClimbSystemCharacter::TurnToWallRightCorner()
{
	if (bCharacterIsHanging)
	{
		if (!bCanJumpRight && bCanTurnRight)
		{
			DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			
			PlayAnimMontage(CornerRightMontage, 1.0f, NAME_None);

			FLatentActionInfo LatentInfo;
			LatentInfo.CallbackTarget			= this;
			LatentInfo.ExecutionFunction		= FName("GrabLedge");
			LatentInfo.Linkage					= 0;
			LatentInfo.UUID						= GiveMeAnUUIDNumber();

			FLatentActionInfo LatentInfoInputs;
			LatentInfoInputs.CallbackTarget		= this;
			LatentInfoInputs.ExecutionFunction	= FName("EnablePlayerInputs");
			LatentInfoInputs.Linkage			= 0;
			LatentInfoInputs.UUID				= GiveMeAnUUIDNumber();

			UKismetSystemLibrary::Delay(GetWorld(), 0.8f, LatentInfo);
			UKismetSystemLibrary::Delay(GetWorld(), 1.5f, LatentInfoInputs);
		}
	}
}

void AClimbSystemCharacter::EnablePlayerInputs()
{
	EnableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}

#pragma endregion

#pragma region Jump Up

void AClimbSystemCharacter::JumpUpTracer()
{
	FHitResult HitResult;
	const FCollisionShape MyCapsule = FCollisionShape::MakeCapsule(20.0f, 100.0f);

	const bool bOnHit =	GetWorld()->SweepSingleByChannel(HitResult, UpArrow->GetComponentLocation(),
						UpArrow->GetComponentLocation(), FQuat::Identity, ECC_GameTraceChannel1, MyCapsule);

	bCanJumpUp = bOnHit ? true : false;
}

void AClimbSystemCharacter::JumpUp_Implementation(bool bJumpUp)
{
	bIsJumping = bJumpUp;
	GetCharacterMovement()->StopMovementImmediately();
	bIsJumping = false;
	
	GrabLedge();
	EnablePlayerInputs();
}

void AClimbSystemCharacter::JumpUpLedge()
{
	if (GetInputAxisValue("MoveRight") == 0 && bCanJumpUp && !bIsJumping)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
			IClimbInterface::Execute_JumpUp(MyCharacterMesh->GetAnimInstance(), true);

		bIsJumping = true;

		DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	}
}

#pragma  endregion

#pragma region TurnBack And JumpBack

void AClimbSystemCharacter::CharacterTurnBack()
{
	bTurnedBack = true;

	if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
		IClimbInterface::Execute_TurnBack(MyCharacterMesh->GetAnimInstance(), true);
}

void AClimbSystemCharacter::CharacterTurnForward()
{
	if (bTurnedBack)
	{
		if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
			IClimbInterface::Execute_TurnBack(MyCharacterMesh->GetAnimInstance(), false);

		bTurnedBack = false;
	}
}

void AClimbSystemCharacter::JumpBack()
{
	const FVector LaunchCharacterVelocity = (GetActorForwardVector() * -500) + FVector(0,0,700.0f);
	
	LaunchCharacter(LaunchCharacterVelocity, false, false);
	ExitClimb();

	if (MyCharacterMesh->GetAnimInstance()->GetClass()->ImplementsInterface(UClimbInterface::StaticClass()))
	IClimbInterface::Execute_TurnBack(MyCharacterMesh->GetAnimInstance(), false);

	bTurnedBack = false;

	const FRotator NewCharacterRotation =	UKismetMathLibrary::MakeRotator(GetActorRotation().Roll, 
											GetActorRotation().Pitch, GetActorRotation().Yaw - 180.0f);

	SetActorRotation(NewCharacterRotation);
}

#pragma endregion

int32 AClimbSystemCharacter::GiveMeAnUUIDNumber()
{
	if (currentUUIDValue > maxUUIDValues)
	{
		currentUUIDValue = 0;
		return currentUUIDValue;
	}
	else
		return currentUUIDValue++;
}
