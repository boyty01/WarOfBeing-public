// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/WobGasVrCharacter.h"
#include "ActorComponent/WOBAbilityComponent.h"
#include "AttributeSet/WobGasAttributes.h"
#include "GripMotionControllerComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "SceneComponent/HandComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "ActorComponent/PhysicsHandComponent.h"
#include "Misc/VREPhysicsConstraintComponent.h"
#include "Ability/WobGasAbilityBase.h"

// Sets default values
AWobGasVrCharacter::AWobGasVrCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Ability system setup 
	AbilitySystemComponent = CreateDefaultSubobject<UWOBAbilityComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	Attributes = CreateDefaultSubobject<UWobGasAttributes>(TEXT("Attributes"));

	// right VR hand setup
	RightHandPhysicsComponent = CreateDefaultSubobject<UPhysicsHandComponent>(TEXT("RightHandComponent"));
	RightHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Right Hand Mesh"));
	RightHandMesh->SetupAttachment(RightMotionController);
	RightHandPhysRoot = CreateDefaultSubobject<USphereComponent>(TEXT("RightHandPhysRoot"));
	RightHandPhysRoot->SetupAttachment(RightMotionController);
	RightAimComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Right Aim Component"));
	RightAimComponent->SetupAttachment(RightMotionController);

	// left VR hand setup
	LeftHandPhysicsComponent = CreateDefaultSubobject<UPhysicsHandComponent>(TEXT("LeftHandComponent"));
	LeftHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Left Hand Mesh"));
	LeftHandMesh->SetupAttachment(LeftMotionController);
	LeftHandPhysRoot = CreateDefaultSubobject<USphereComponent>(TEXT("LeftHandPhysRoot"));
	LeftHandPhysRoot->SetupAttachment(LeftMotionController);
	LeftAimComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Left Aim Component"));
	LeftAimComponent->SetupAttachment(LeftMotionController);

}

UAbilitySystemComponent* AWobGasVrCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AWobGasVrCharacter::InitialiseAttributes()
{
	if (AbilitySystemComponent && DefaultAttributeEffect)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1, EffectContext);

		if (SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle EffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void AWobGasVrCharacter::GiveAbilities()
{
	if (HasAuthority() && AbilitySystemComponent)
	{
		for (TSubclassOf<UWobGasAbilityBase>& StartupAbility : DefaultAbilities)
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(StartupAbility, 1, static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputId), this));
		}
	}
}

void AWobGasVrCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		InitialiseAttributes();
		GiveAbilities();
	}
}

void AWobGasVrCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitialiseAttributes();

	if (AbilitySystemComponent && InputComponent)
	{
		const FGameplayAbilityInputBinds Binds("Confirm", "Cancel", "EGasAbilityInputId", static_cast<int32>(EGasAbilityInputId::Confirm), static_cast<int32>(EGasAbilityInputId::Cancel));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
	}

}



void AWobGasVrCharacter::BindAttributeListeners() const
{
	AbilitySystemComponent->OnZeroHealth.AddDynamic(this, &AWobGasVrCharacter::OnDeath);
}

void AWobGasVrCharacter::OnHandStartedClimb(bool& bIsRightHand, UObject* ClimbObject)
{
	UE_LOG(LogTemp, Warning, TEXT("A Hand has started climbing %s"), RightHandPhysicsComponent->IsClimbing() || LeftHandPhysicsComponent->IsClimbing() ? TEXT("TRUE") : TEXT("FALSE"));
	if (!VRMovementReference) return;
	
	bIsClimbing = true;
	ClimbingController = bIsRightHand ? RightMotionController : LeftMotionController;
	// set climbing mode to true if either hand appears to be climbing

	VRMovementReference->SetClimbingMode(true);
	return;

}

void AWobGasVrCharacter::OnHandEndedClimb(bool& bIsRightHand)
{
	if (!VRMovementReference) return;
	UE_LOG(LogTemp, Warning, TEXT("A Hand has ended climbing"));

	UPhysicsHandComponent* OtherHand = bIsRightHand ? LeftHandPhysicsComponent : RightHandPhysicsComponent;

	if (OtherHand->IsClimbing())
	{
		ClimbingController = bIsRightHand ? LeftMotionController : RightMotionController;
		return;
	}

	bIsClimbing = !RightHandPhysicsComponent->IsClimbing() && !LeftHandPhysicsComponent->IsClimbing();
	
	if (!bIsClimbing)
	{
		ClimbingController = nullptr;
		// set climbing mode to false if neither hand appears to be climbing.
		VRMovementReference->SetClimbingMode(!RightHandPhysicsComponent->IsClimbing() && !LeftHandPhysicsComponent->IsClimbing());
	}
	return;
}

TEnumAsByte<EControllerHand> AWobGasVrCharacter::GetAnimOwningControllerHand(UAnimInstance* TargetAnimInstance)
{
	if (LeftHandMesh->GetAnimInstance() == TargetAnimInstance) return EControllerHand::Left;

	if (RightHandMesh->GetAnimInstance() == TargetAnimInstance) return EControllerHand::Right;

	return EControllerHand::Special;
}

UAnimSequence* AWobGasVrCharacter::GetCurrentAnimationSequence(const EControllerHand Hand)
{
	if (Hand == EControllerHand::Left) return LeftHandPhysicsComponent->GetCurrentAnimation();

	if (Hand == EControllerHand::Right) return RightHandPhysicsComponent->GetCurrentAnimation();

	return nullptr;
}

// Called when the game starts or when spawned
void AWobGasVrCharacter::BeginPlay()
{
	Super::BeginPlay();

	BindAttributeListeners();
	
	// init the hand components
	RightHandPhysicsComponent->InitHandComponent(RightMotionController, LeftMotionController,RightHandPhysRoot, RightAimComponent, RightHandMesh, LeftHandPhysicsComponent) ;

	LeftHandPhysicsComponent->InitHandComponent(LeftMotionController, RightMotionController, LeftHandPhysRoot, LeftAimComponent, LeftHandMesh, RightHandPhysicsComponent);

	RightHandPhysicsComponent->OnStartClimbing.AddDynamic(this, &AWobGasVrCharacter::OnHandStartedClimb);
	RightHandPhysicsComponent->OnEndClimbing.AddDynamic(this, &AWobGasVrCharacter::OnHandEndedClimb);

	LeftHandPhysicsComponent->OnStartClimbing.AddDynamic(this, &AWobGasVrCharacter::OnHandStartedClimb);
	LeftHandPhysicsComponent->OnEndClimbing.AddDynamic(this, &AWobGasVrCharacter::OnHandEndedClimb);
	//RightMotionController->SetCustomPivotComponent(RightHandPhysRoot);
	//LeftMotionController->SetCustomPivotComponent(LeftHandPhysRoot);
}

bool AWobGasVrCharacter::TraceAndTryGrip(UGripMotionControllerComponent* GripComponent)
{
	if (!GetWorld() || !GripComponent) return false;
	
	FVector TraceStart = GripComponent->GetComponentLocation();
	FVector TraceEnd = GripComponent->GetComponentLocation() + GripComponent->GetForwardVector() * GripTraceDistance;
	TArray<FHitResult> Results;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	float SphereRadius = 10.0f;
	TArray<AActor*> IgnoredActors;
	UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(), 
		TraceStart, 
		TraceEnd, 
		SphereRadius, 
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel1), 
		false, 
		IgnoredActors, 
		bDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, 
		Results, 
		true
	);
//	GetWorld()->LineTraceSingleByChannel(Result, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, Params);

	for (auto& Result : Results)
	{

		FName BoneToGrip = FName();
		FName SocketToSnap = FName("GripSocket");
		bool bIsSlotGrip = true;


		FTransform GripSlotWorldLoc;
		bool bSlotInRange;
		FName SlotName;
		// try to grip the hit component
		if (Result.GetComponent()->Implements<UVRGripInterface>())
		{
			IVRGripInterface::Execute_ClosestGripSlotInRange(
				Result.GetComponent(),
				Result.GetComponent()->GetComponentTransform().GetLocation(),
				false,
				bSlotInRange,
				GripSlotWorldLoc,
				SlotName,
				GripComponent,
				FName()
			);

			if (bSlotInRange)
			{
				if (GripComponent->GripObjectByInterface(
					Result.GetComponent(),
					UKismetMathLibrary::MakeRelativeTransform(Result.GetComponent()->GetComponentTransform(), GripSlotWorldLoc),
					true,
					BoneToGrip,
					SlotName,
					bIsSlotGrip))
				{
					USkeletalMeshComponent* HandMesh = Cast<USkeletalMeshComponent>(GripComponent->GetChildComponent(0));
					if (HandMesh)
					{
						HandMesh->SetWorldLocation(GripSlotWorldLoc.GetLocation());
					} 
				}
			}
		}


		// try to grip the hit actor 
		if (Result.GetActor()->Implements<UVRGripInterface>())
		{
			IVRGripInterface::Execute_ClosestGripSlotInRange(
				Result.GetActor(),
				Result.GetActor()->GetActorLocation(),
				false,
				bSlotInRange,
				GripSlotWorldLoc,
				SlotName,
				GripComponent,
				FName()
			);

			if (bSlotInRange)
			{
				if(GripComponent->GripObjectByInterface(
					Result.GetActor(),
					UKismetMathLibrary::MakeRelativeTransform(Result.GetActor()->GetActorTransform(), GripSlotWorldLoc),
					true,
					BoneToGrip,
					SocketToSnap,
					bIsSlotGrip))
				{
					USkeletalMeshComponent* HandMesh = Cast<USkeletalMeshComponent>(GripComponent->GetChildComponent(0));
					if (HandMesh)
					{
						HandMesh->SetWorldLocation(GripSlotWorldLoc.GetLocation());
					}
				}
			}

		}
	}
	return false;
}

void AWobGasVrCharacter::VRMoveForward(const float val)
{
	AddMovementInput(GetVRForwardVector(), val);
}

void AWobGasVrCharacter::VRStrafeRight(const float val)
{
	AddMovementInput(GetVRRightVector(), val);
}

void AWobGasVrCharacter::VRTurnRight(const float val)
{
	VRMovementReference->PerformMoveAction_SnapTurn(val, EVRMoveActionVelocityRetention::VRMOVEACTION_Velocity_None, true);
}

// Called every frame
void AWobGasVrCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClimbing && ClimbingController)
	{
		USkeletalMeshComponent* DominantHand = ClimbingController == RightMotionController ? RightHandMesh : LeftHandMesh;
		const FVector ClimbingOffset = DominantHand->GetComponentLocation() - ClimbingController->GetComponentLocation();;
		VRMovementReference->AddCustomReplicatedMovement(ClimbingOffset);
	}

}

// Called to bind functionality to input
void AWobGasVrCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	if (AbilitySystemComponent && InputComponent)
	{
		const FGameplayAbilityInputBinds Binds("Confirm", "Cancel", "EGasAbilityInputId", static_cast<int32>(EGasAbilityInputId::Confirm), static_cast<int32>(EGasAbilityInputId::Cancel));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
	}
}


