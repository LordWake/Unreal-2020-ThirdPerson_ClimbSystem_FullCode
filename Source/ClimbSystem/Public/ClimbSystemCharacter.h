//+---------------------------------------------------------+
//| Project   : MedelDesign Climb System C++ UE 4.24		|
//| Author    : github.com/LordWake					 		|		
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "ClimbInterface.h"
#include "GameFramework/Character.h"
#include "Components/ArrowComponent.h"
#include "ClimbSystemCharacter.generated.h"

UCLASS(config=Game)
class AClimbSystemCharacter : public ACharacter, public IClimbInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CustomMovement, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* RightArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CustomMovement, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* LeftArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CustomMovement, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* RightLedge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CustomMovement, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* LeftLedge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CustomMovement, meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* UpArrow;

public:
	AClimbSystemCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:

	UPROPERTY(BlueprintReadWrite)
	bool bCanMoveLeft;

	UPROPERTY(BlueprintReadWrite)
	bool bCanMoveRight;

	UPROPERTY(BlueprintReadWrite)
	bool bMovingLeft;

	UPROPERTY(BlueprintReadWrite)
	bool bMovingRight;
	
	//*******************************************************************************************************************
	//		CLIMB WALL                       
	//*******************************************************************************************************************
	
	/* Creates a forward collision trace Sphere and saves HitLocation and Normal*/
	void ForwardTracer();
	/* Creates a seconds sphere collision and check if we can Jump to the wall*/
	void HeightTracer();
	/* Sets some variables when the player is hanging from the ledge*/
	void ClimbLedge();
	/* Take the Player off the wall*/
	void ExitClimb();
	/* Sets some variables when the player's already climb the wall */
	void CharacterClimbLedge_Implementation(bool bCharacterIsClimbing) override;
	/* Stops Player's Movement when reach the top of the wall. Used in LatentInfo*/
	UFUNCTION(BlueprintCallable)
	void LedgeMovementFinished();	
	/* Raise the player to the top of the Wall using MoveCompontentTo from KismetSystemLibrary. Used in LatentInfo*/
	UFUNCTION(BlueprintCallable)
	void GrabLedge(); 	

	//*******************************************************************************************************************
	//		MOVE TO THE SIDES WHILE HANGING                        
	//*******************************************************************************************************************
	
	/* Checks the Left and Right Tracers on the Tick*/
	void MoveSides();
	/* Create two capsule collisions from the arrow components to check if we still have a wall to move around */
	void RightLeftTracer(const bool& bRight);
	/* Sets some variables when the player is moving around the wall*/
	void MoveCharacterOnTheSides(const bool& bMoving);
	/* Interpolates character's position from where we are, to next target position in wall*/
	void MoveInLedge();
	
	//*******************************************************************************************************************
	//		JUMP                         
	//*******************************************************************************************************************
	
	/* Checks the Left and Right Tracers on the Tick*/
	void CheckForJumpOnTheSides();
	/* Creates two capsule collisions from the Ledge Arrow components to check if we can jump*/
	void JumpRightLeftTracer(const bool& bRight);
	/* Sets some variables when the Animator blueprint stops the montage*/
	void JumpRight_Implementation(bool bJumpRight) override;
	/* Sets some variables when the Animator blueprint stops the montage*/
	void JumpLeft_Implementation(bool bJumpLeft) override;
	/*Called from CheckJump. This function actually makes the player jumps to the side wall
	It has a LatenInfo that after a Delay function, it calls the GrabLedge function.*/
	void JumpRightLeftLedge(const bool& bRight);	
	
	//*******************************************************************************************************************
	//		CORNER DETECTION                        
	//*******************************************************************************************************************
	
	/* Creates two Sphere collisions from the Right/Left Arrow components to check if we can turn around the corner.*/
	void TurnCornerRightLeftTracer(const bool& bRight);
	/* Turns Left the corner and play an Animation Montage.*/
	void TurnToWallLeftCorner();
	/* Turns Right the corner and play an Animation Montage.*/
	void TurnToWallRightCorner();
	/* Enables player's Input after turn around the corner. Used in LatentInfo*/
	UFUNCTION(BlueprintCallable)
	void EnablePlayerInputs();

	//*******************************************************************************************************************
	//		JUMP UP                       
	//*******************************************************************************************************************

	/* Creates a capsule collision from the Up Arrow components to check if we can jump Up*/
	void JumpUpTracer();
	/* Creates a capsule collision from the Up Arrow components to check if we can jump Up*/
	void JumpUpLedge();
	/* Calls the AnimBlueprint interface JumpUp to make the Jump*/
	void JumpUp_Implementation(bool bJumpUp) override;

	//*******************************************************************************************************************
	//		TURN BACK & JUMP BACK                     
	//*******************************************************************************************************************
	
	/* Calls the AnimBlueprint to make the player turn back while hanging*/
	void CharacterTurnBack();
	/* Calls the AnimBlueprint to make the player turn forward while hanging if it was turned back*/
	void CharacterTurnForward();
	/* Launch the character backwards*/
	void JumpBack();

	//*******************************************************************************************************************
	//		UE4 Functions                       
	//*******************************************************************************************************************

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick( float DeltaSeconds ) override;
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditDefaultsOnly, Category = AnimMontages)
	UAnimMontage* CornerLeftMontage;

	UPROPERTY(EditDefaultsOnly, Category = AnimMontages)
	UAnimMontage* CornerRightMontage;

	USkeletalMeshComponent* MyCharacterMesh;

	FVector WallLocation;
	FVector WallNormal;
	FVector WallHeightLocation;

	bool bCharacterIsHanging	= false;
	bool bTurnedBack			= false;
	bool bIsJumping				= false;
	bool bIsClimbingLedge;
	bool bCanJumpUp;
	bool bCanJumpLeft;
	bool bCanJumpRight;
	bool bCanTurnLeft;
	bool bCanTurnRight;

	const float moveSidesSpeed	= 17.0f;

	const int32 maxUUIDValues	= 25;
	int32 currentUUIDValue		= -1;
	
	/* Checks if it should do a Normal Jump, a Jump on the sides or climb when we press the Jump Input*/
	void CheckForJump();
	/* Checks if it should turn back or fall down*/
	void CheckForTurnBackOrExit();

	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);

	int32 GiveMeAnUUIDNumber();
};

