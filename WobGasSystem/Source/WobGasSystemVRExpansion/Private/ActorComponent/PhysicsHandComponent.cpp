// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/PhysicsHandComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "VRGripInterface.h"
#include "VRBaseCharacter.h"
#include "GripMotionControllerComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ActorComponent/WobGripHandSocketComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Animation/SkeletalMeshActor.h"
#include "GameplayTagAssetInterface.h"
#include "VRExpansionFunctionLibrary.h"
#include "Character/WobGasVrCharacter.h"
#include "Interface/WobVrGripInterface.h"
#include "Grippables/HandSocketComponent.h"
#include "Animation/PoseSnapshot.h"
#include "Misc/VREPhysicsConstraintComponent.h"


#define PRINT(s) if(GEngine) UE_LOG(LogTemp, Warning, TEXT(s));

// Sets default values for this component's properties
UPhysicsHandComponent::UPhysicsHandComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

bool UPhysicsHandComponent::TriggerGripOrDrop(const bool bIsGrip)
{
	FGameplayTagContainer RelevantTags;

	// if Grip is pressed
	if (bIsGrip)
	{
		RelevantTags.AddTag(FGameplayTag::RequestGameplayTag(FName("DropType.OnPrimaryGrip")));
		RelevantTags.AddTag(FGameplayTag::RequestGameplayTag(FName("DropType.Secondary.OnPrimaryGrip")));
		RelevantTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GripType.OnPrimaryGrip")));
		RelevantTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GripType.Secondary.OnPrimaryGrip")));
		return GripOrDropObject(RelevantTags, true);
	}

	// Otherwise, if we were climbing, clear the setup.
	if (bIsClimbing)
	{
		OnEndClimbing.Broadcast(bIsRightHand);
		bIsClimbing = false;
		DetachFromObject(FBPActorGripInformation());
		return true;
	}

	// Otherwise just release grip
	RelevantTags.AddTag(FGameplayTag::RequestGameplayTag(FName("DropType.OnPrimaryGripRelease")));
	RelevantTags.AddTag(FGameplayTag::RequestGameplayTag(FName("DropType.Secondary.OnPrimaryGripRelease")));
	return GripOrDropObject(RelevantTags, false);

	return false;
}

void UPhysicsHandComponent::InitHandComponent(UGripMotionControllerComponent* CComp, UGripMotionControllerComponent* OtherCComp, UPrimitiveComponent* TraceComp, USceneComponent* AimComp, USkeletalMeshComponent* HandMesh, UPhysicsHandComponent* _OtherHandComponent)
{
	ThisHand = CComp;
	OtherHand = OtherCComp;
	CollisionSphere = TraceComp;
	AimComponent = AimComp;
	HandMeshComponent = HandMesh;
	MeshBaseTransform = HandMesh->GetRelativeTransform();
	OtherHandComponent = _OtherHandComponent;
	ThisHand->OnDroppedObject.AddDynamic(this, &UPhysicsHandComponent::OnDropped);
	ThisHand->OnGrippedObject.AddDynamic(this, &UPhysicsHandComponent::OnGrabbed);
	ThisHand->OnSecondaryGripAdded.AddDynamic(this, &UPhysicsHandComponent::OnSecondaryGripAdded);
	ThisHand->OnSecondaryGripRemoved.AddDynamic(this, &UPhysicsHandComponent::OnSecondaryGripRemoved);
}

void UPhysicsHandComponent::OnSecondaryGripAddedOnOther(const FBPActorGripInformation GripInfo)
{
	bIsSecondaryGripping = true;
	RetrievePoses(GripInfo);
	InitialiseAndAttach(GripInfo, true, bHasCustomAnimation);
}

bool UPhysicsHandComponent::GripOrDropObject(const FGameplayTagContainer RelevantTags, const bool bCanCheckClimb)
{
	
	// if we have gripped objects, drop them and return.
	if (ThisHand->HasGrippedObjects())
	{
		DropItems(RelevantTags);
		return true;
	}

	if (DropSecondaryAttachment(RelevantTags)) return true;


	UObject* NearestObject = nullptr;
	bool bImplementsInterface;
	FTransform ObjectTransform;
	bool bCanBeClimbed;
	FName BoneName;
	FVector ImpactLocation;

	GetNearestOverlappingObject(NearestObject, bImplementsInterface, ObjectTransform, bCanBeClimbed, BoneName, ImpactLocation, RelevantTags);
	ObjectTransform.NormalizeRotation();
	// return if object is invalid
	if (!NearestObject)
	{
		return false;
	}

	// climbing checks.  If the target has the climbable tag, then we will try to climb it. Regardless of whether this succeeds or fails, further grip logic will 
	// not run.  This is specifically set so that climbable objects are not also grippable, rendering all other grip tags irrelevant if GripSockets.Climbable is set.
	bCanBeClimbed = Cast<IGameplayTagAssetInterface>(NearestObject)->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("GripSockets.Climbable")));
	if (bCanBeClimbed)
	{
		return TryClimb(NearestObject, bCanCheckClimb);
	}

	if (bImplementsInterface)
	{
		if(!CanAttemptGrabOnObject(NearestObject)) return false;

		ESecondaryGripType SecGripType;
		// if it can be secondary gripped, try that first.
		if (CanAttemptSecondaryGrabOnObject(NearestObject, SecGripType))
		{
			
			FGameplayTag DefSecGripTag = FGameplayTag::RequestGameplayTag(FName("GripType.Secondary.OnPrimaryGrip"));
			if (ValidateGameplayTagContainer(FGameplayTag::RequestGameplayTag("GripType.Secondary"), NearestObject, DefSecGripTag, RelevantTags))
			{
				FTransform SlotWorldTransform;
				bool bHadSlotInRange;
				FName SlotName;
				IVRGripInterface::Execute_ClosestGripSlotInRange(
					NearestObject,
					ImpactLocation,
					true,
					bHadSlotInRange,
					SlotWorldTransform,
					SlotName,
					ThisHand,
					NAME_None
				);
				
				FTransform GripTransform = bHadSlotInRange ? ThisHand->ConvertToGripRelativeTransform(ObjectTransform, SlotWorldTransform) : ThisHand->ConvertToGripRelativeTransform(ObjectTransform, ThisHand->GetPivotTransform());
				FGameplayTag FoundTag;
				UVRExpansionFunctionLibrary::GetFirstGameplayTagWithExactParent(FGameplayTag::RequestGameplayTag(FName("GripType.Secondary")), RelevantTags, FoundTag);

				if (CanSecondaryGripObject(NearestObject, bHadSlotInRange, SecGripType, FoundTag))
				{
					TryGrab(NearestObject, bHadSlotInRange, GripTransform, NAME_None, SlotName, true, FoundTag);
					return true;
				}
			}
			
		} 

		// primary grip interface
		if (IVRGripInterface::Execute_DenyGripping(NearestObject, ThisHand)) return false;

		if (!ValidateGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("GripType")), NearestObject, FGameplayTag::RequestGameplayTag(FName("GripType.OnPrimaryGrip")), RelevantTags)) return false;


		FTransform SlotWorldTransform;
		bool bHadSlotInRange;
		FName SlotName;
		EControllerHand HandType;
		ThisHand->GetHandType(HandType);
		FName OverridePrefix = GetCorrectPrimarySlotPrefix(NearestObject, HandType, BoneName);
		IVRGripInterface::Execute_ClosestGripSlotInRange(
			NearestObject,
			ImpactLocation,
			false,
			bHadSlotInRange,
			SlotWorldTransform,
			SlotName,
			ThisHand,
			OverridePrefix
		);

		// if no slot and doesnt allow free grip then fail out.
		if (!bHadSlotInRange && Cast<IGameplayTagAssetInterface>(NearestObject)->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Weapon.DenyFreeGripping")))) return false;


		FTransform NonRelativeGripTransform = BoneName == NAME_None ? ObjectTransform : GetBoneTransform(NearestObject, BoneName);	
		FTransform RelativeGripTransform = UKismetMathLibrary::MakeRelativeTransform(NonRelativeGripTransform, SlotWorldTransform);
		FTransform NonGripTransform = ThisHand->ConvertToControllerRelativeTransform(NonRelativeGripTransform); 
		RemoveControllerScale(RelativeGripTransform);
		TryGrab(
			NearestObject,
			true,
			bHadSlotInRange ? RelativeGripTransform : NonGripTransform,
			BoneName,
			SlotName,
			false
		);

		return true;
	}

	// try default grip 
	FTransform NonRelativeGripTransform = BoneName == NAME_None ? ObjectTransform : GetBoneTransform(NearestObject, BoneName);
	FTransform NonGripTransform = ThisHand->ConvertToControllerRelativeTransform(NonRelativeGripTransform);  NonGripTransform.NormalizeRotation();
	if(!RelevantTags.HasTagExact(FGameplayTag::RequestGameplayTag(FName("GripType.OnPrimaryGrip")))) return false;

	TryGrab(
		NearestObject,
		false,
		NonGripTransform,
		BoneName,
		NAME_None,
		false,
		FGameplayTag::EmptyTag
	);

	return true;
}

void UPhysicsHandComponent::DropItems(const FGameplayTagContainer GameplayTags)
{

	TArray<FBPActorGripInformation> Grips;
	ThisHand->GetAllGrips(Grips);

	for(auto &Grip : Grips)
	{
		if (!Grip.GrippedObject->IsValidLowLevelFast()) return;

		// validate the tag exists
		FGameplayTag BaseTag = FGameplayTag::RequestGameplayTag(FName("DropType"));
		FGameplayTag DefaultDropTag = FGameplayTag::RequestGameplayTag(FName("DropType.OnPrimaryGripRelease"));
		
		if (!ValidateGameplayTagContainer(BaseTag, Grip.GrippedObject, DefaultDropTag, GameplayTags)) return;


		if (Grip.GrippedObject->Implements<UVRGripInterface>())
		{

			// check if the item we're dropping wants to socket with something
			USceneComponent* TargetSocket;
			FName SocketName;
			FTransform_NetQuantize RelativeTransform;
			if (IVRGripInterface::Execute_RequestsSocketing(Grip.GrippedObject, TargetSocket, SocketName, RelativeTransform))
			{
				ThisHand->DropAndSocketGrip(Grip, TargetSocket, SocketName, RelativeTransform);
				return;
			}

			// otherwise just drop it with velocity calculated. 
			FVector CurAngular;
			FVector CurLinear;
			ThisHand->GetPhysicsVelocities(Grip, CurAngular, CurLinear);
			FVector FinalAngular;
			FVector FinalLinear;
			GetObjectThrowVelocity(Grip, CurAngular, CurLinear, FinalAngular, FinalLinear);
			ThisHand->DropObjectByInterface(Grip.GrippedObject, Grip.GripID, FinalAngular, FinalLinear);
			return;
		}

		//catch all just in case something manages not to implement the interface. doesn't calculate velocity!
		ThisHand->DropGrip(Grip);
		return;
	}
}

bool UPhysicsHandComponent::DropSecondaryAttachment(const FGameplayTagContainer GameplayTags)
{
	if (!OtherHand) 
	{
		return false;
	}

	FBPActorGripInformation GripInfo;
	if (!OtherHand->GetIsSecondaryAttachment(ThisHand, GripInfo)) return false;


	if(!ValidateGameplayTagContainer(FGameplayTag::RequestGameplayTag("DropType.Secondary"), GripInfo.GrippedObject, FGameplayTag::RequestGameplayTag("DropType.Secondary.OnPrimaryGripRelease"), GameplayTags)) return false;
	
	OtherHand->RemoveSecondaryAttachmentPoint(GripInfo.GrippedObject);
	return true;

}

bool UPhysicsHandComponent::GetNearestOverlappingObject(UObject*& NearestObject, bool& bImplementsInterface, FTransform& ObjectTransform, bool& bCanBeClimbed, FName& BoneName, FVector& ImpactLocation, const FGameplayTagContainer RelevantTags)
{
	if (!CollisionSphere)
	{
		PRINT("No overlap component exists.");
		return false;
	}

	UPrimitiveComponent* FirstHitPrimitive;

	// trace for nearest. 
	if (!bForceOverlapOnlyGripChecks)
	{
		if (TraceForNearestObject(NearestObject, bImplementsInterface, ObjectTransform, bCanBeClimbed, BoneName, ImpactLocation, RelevantTags, FirstHitPrimitive)) return true;

		if (FirstHitPrimitive)
		{
			// check climb
		}
		return false;
	}


	// check overlap
	TArray<UPrimitiveComponent*> OutComponents;
	TArray<TEnumAsByte<EObjectTypeQuery>> CollisionsToCheck{ 
		EObjectTypeQuery::ObjectTypeQuery1,
		EObjectTypeQuery::ObjectTypeQuery2,
		EObjectTypeQuery::ObjectTypeQuery3,
		EObjectTypeQuery::ObjectTypeQuery4,
		EObjectTypeQuery::ObjectTypeQuery5,
		EObjectTypeQuery::ObjectTypeQuery6 
	};
	TArray<AActor*> Ignored{ GetOwner()};
	bool bComponentHit = UKismetSystemLibrary::ComponentOverlapComponents(CollisionSphere, CollisionSphere->GetComponentTransform(), CollisionsToCheck, nullptr, Ignored, OutComponents);

	if (!bComponentHit) return false;

	FirstHitPrimitive = OutComponents[0];
	uint8 LastGripPrio = 0;
	uint8 OutGripPrio = 0;
	FName Bone = NAME_None;
	UObject* ObjToGrip;
	bool bDoesImplementInterface;
	FTransform CurObjTransform;
	for (auto& Overlap : OutComponents)
	{
		if (Overlap->GetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility) == ECollisionResponse::ECR_Ignore) return false;

		if (ShouldGripComponent(Overlap, LastGripPrio, Overlap != FirstHitPrimitive, Bone, RelevantTags, ObjToGrip, bDoesImplementInterface, CurObjTransform, OutGripPrio))
		{
			NearestObject = ObjToGrip;
			bImplementsInterface = bDoesImplementInterface;
			ObjectTransform = CurObjTransform;
			ImpactLocation = Overlap->GetComponentLocation();
			LastGripPrio = OutGripPrio;
		}
	}

	// check climb if we havent managed to grip anything.
	if (!NearestObject)
	{
		// try component
		if (FirstHitPrimitive->Implements<UWobVrGripInterface>())
		{
			bCanBeClimbed =IWobVrGripInterface::Execute_CanBeClimbed(FirstHitPrimitive);
			return true;
		}

		// try owner
		if (FirstHitPrimitive->GetOwner()->Implements<UWobVrGripInterface>())
		{
			bCanBeClimbed = IWobVrGripInterface::Execute_CanBeClimbed(FirstHitPrimitive->GetOwner());
			return true;
		}
	}

	return true;
}

bool UPhysicsHandComponent::TraceForNearestObject(UObject*& NearestObject, bool& bImplementsInterface, FTransform& ObjectTransform, bool& bCanBeClimbed, FName& BoneName, FVector& ImpactLocation, const FGameplayTagContainer RelevantTags, UPrimitiveComponent*& FirstPrimitive)
{
	FVector Origin;
	FVector BoxExtent;
	float Radius;
	UKismetSystemLibrary::GetComponentBounds(CollisionSphere, Origin, BoxExtent, Radius);
	FVector ForwardVectorOffset = (UKismetMathLibrary::GetForwardVector(AimComponent->GetComponentRotation()) * GripTraceLength);
	TArray<AActor*> GrippedActors; ThisHand->GetGrippedActors(GrippedActors);
	FVector TraceStart = Origin - ForwardVectorOffset;
	FVector TraceEnd = Origin + ForwardVectorOffset;
	TArray<FHitResult> OutHits;

	bool bHasHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		TraceStart,
		TraceEnd,
		Radius,
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
		true,
		GrippedActors,
		EDrawDebugTrace::None,
		OutHits,
		true
	);

	if (!bHasHit) return false;

	UPrimitiveComponent* FirstHitPrimitive = OutHits[0].GetComponent();
	bool bHasGrippable = false;
	uint8 BestGripPrio = 0;
	uint32 Count = 0;
	bool bDoesImplementInterface;
	for (auto& Hit : OutHits)
	{
		FTransform OutTransform;
		uint8 ThisGripPrio = 0;
		UObject* ShouldGripObject;
		if (ShouldGripComponent(Hit.GetComponent(), BestGripPrio, Count > 0, Hit.BoneName, RelevantTags, ShouldGripObject, bDoesImplementInterface, OutTransform, ThisGripPrio))
		{
			BestGripPrio = ThisGripPrio;
			NearestObject = ShouldGripObject;
			BoneName = Hit.BoneName;
			ObjectTransform = OutTransform;
			bImplementsInterface = bDoesImplementInterface;
			ImpactLocation = Hit.ImpactPoint;
			bHasGrippable = true;
		}
	}

	if (bHasGrippable)
	{
		if (NearestObject->Implements<UVRGripInterface>())
		{
			if (!Cast<IGameplayTagAssetInterface>(NearestObject)->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Interactible.PerBoneGripping"))))
			{
				BoneName = NAME_None;
			}
		}

	}

	FirstPrimitive = FirstHitPrimitive;
	return true;
}

bool UPhysicsHandComponent::ShouldGripComponent(UPrimitiveComponent* ComponentToCheck, uint8& GripPrioToCheckAgainst, bool bCheckAgainstPrior, FName& BoneName, const FGameplayTagContainer RelevantTags, UObject*& ObjectToGrip, bool& bImplementsInterface, FTransform& ObjectWorldTransform, uint8& GripPrio)

{ 
	// init all outputs to defaults 
	GripPrio = 0; ObjectWorldTransform = FTransform(); bImplementsInterface = false;

	// fail if it ignores visibility.
	if (ComponentToCheck->GetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility) == ECollisionResponse::ECR_Ignore) return false;

	// if hit component implements interface
	if (ComponentToCheck->Implements<UVRGripInterface>())
	{
		if (IVRGripInterface::Execute_DenyGripping(ComponentToCheck, ThisHand) && IVRGripInterface::Execute_SecondaryGripType(ComponentToCheck) == ESecondaryGripType::SG_None)
		{
			return false;
		}

		uint8 NewPrio;
		if (!CheckGripPriority(ComponentToCheck, bCheckAgainstPrior, GripPrioToCheckAgainst, NewPrio))
		{
			return false;
		}

		if (CheckIsValidForGripping(ComponentToCheck, RelevantTags))
		{
			bImplementsInterface = true; ObjectWorldTransform = ComponentToCheck->GetComponentTransform(); GripPrio = NewPrio; ObjectToGrip = ComponentToCheck;
			return true;
		}

		return false;
	}

	// otherwise, if hit actor implements interface
	AActor* OwningActor = ComponentToCheck->GetOwner();

	// if this is ourself or it doesnt exist, fail out.
	if (OwningActor == GetOwner() || !OwningActor || !OwningActor->IsValidLowLevel()) return false;

	if (OwningActor->Implements<UVRGripInterface>())
	{
		if (IVRGripInterface::Execute_DenyGripping(OwningActor, ThisHand) && IVRGripInterface::Execute_SecondaryGripType(OwningActor) == ESecondaryGripType::SG_None)
		{
			return false;
		}

		uint8 NewGripPrio;
		if (CheckGripPriority(OwningActor, bCheckAgainstPrior, GripPrioToCheckAgainst, NewGripPrio))
		{
			if (CheckIsValidForGripping(OwningActor, RelevantTags))
			{
				bImplementsInterface = true; ObjectWorldTransform = OwningActor->GetActorTransform(); GripPrio = NewGripPrio; ObjectToGrip = OwningActor;
				return true;
			}
			return false;
		}
	}

	if (!bCheckAgainstPrior)
	{
		if (ComponentToCheck->IsSimulatingPhysics(BoneName))
		{
			ObjectToGrip = ComponentToCheck; bImplementsInterface = false; ObjectWorldTransform = ComponentToCheck->GetComponentTransform(); GripPrio = 0;
			return true;
		}
	}

	return false;
}

bool UPhysicsHandComponent::CheckGripPriority(UObject* ObjectToCheck, const bool bCheckAgainstPrior, const uint8 PrioToCheckAgainst, uint8& NewGripPrio)
{
	if (!bCheckAgainstPrior)
	{
		FBPAdvGripSettings GripSettings = IVRGripInterface::Execute_AdvancedGripSettings(ObjectToCheck);
		NewGripPrio = GripSettings.GripPriority;
		return true;		
	}

	FBPAdvGripSettings GripSettings = IVRGripInterface::Execute_AdvancedGripSettings(ObjectToCheck);
	NewGripPrio = GripSettings.GripPriority;

	if (NewGripPrio > PrioToCheckAgainst) return true;

	return false;
}

bool UPhysicsHandComponent::CheckIsClimbable(UObject* NearestObject, bool& bCanClimb)
{
	return false;
}

bool UPhysicsHandComponent::CanAttemptGrabOnObject(UObject* ObjectToCheck)
{
	if (!ObjectToCheck) {
		return false;
	}
	if (ObjectToCheck->Implements<UVRGripInterface>())
	{
		TArray<FBPGripPair> Controllers;
		bool bIsHeld;
		IVRGripInterface::Execute_IsHeld(ObjectToCheck, Controllers, bIsHeld);
		
		// if its already held, and our owner is currently holding it, and it allows multiple grips, output depends who's holding it.
		if (bIsHeld && !IVRGripInterface::Execute_AllowsMultipleGrips(ObjectToCheck))
		{
			UGripMotionControllerComponent* HoldingController = Controllers[0].HoldingController;
			return HoldingController->GetOwner() == GetOwner();
		}

		// otherwise its not held, so we can grab it.
		return true;
	}
	//No interface, no grab.
	return false;
}

bool UPhysicsHandComponent::CanAttemptSecondaryGrabOnObject(UObject* ObjectToCheck, ESecondaryGripType& SecGripType) const
{
	if (!ObjectToCheck->Implements<UVRGripInterface>()) return false;

	TArray<FBPGripPair> HoldingControllers;
	bool bIsHeld;
	IVRGripInterface::Execute_IsHeld(ObjectToCheck, HoldingControllers, bIsHeld);

	// if not held we cant secondary grip
	if (!bIsHeld)
	{
		SecGripType = ESecondaryGripType::SG_None;
		return false;
	}

	SecGripType = IVRGripInterface::Execute_SecondaryGripType(ObjectToCheck);

	return (HoldingControllers[0].HoldingController && HoldingControllers[0].HoldingController->GetOwner() == GetOwner() && SecGripType != ESecondaryGripType::SG_None);
	
}

bool UPhysicsHandComponent::CanSecondaryGripObject(UObject* ObjectToGrip, const bool bHadSlot, const ESecondaryGripType SecGripType, const FGameplayTag GripSecondaryTag) const
{
	if (SecGripType == ESecondaryGripType::SG_None || !ObjectToGrip->Implements<UVRGripInterface>()) return false;

	if (!ValidateGameplayTag(FGameplayTag::RequestGameplayTag(FName("GripType.Secondary")), GripSecondaryTag, ObjectToGrip, FGameplayTag::RequestGameplayTag(FName("GripType.Secondary.OnPrimaryGrip"))) ||
		!ObjectToGrip->Implements<UVRGripInterface>())
	{
		return false;
	}

	if (bHadSlot) return true;

	// reject slot only where no slot available
	if (SecGripType == ESecondaryGripType::SG_SlotOnly ||
		SecGripType == ESecondaryGripType::SG_SlotOnly_Retain ||
		SecGripType == ESecondaryGripType::SG_SlotOnlyWithScaling_Retain ||
		SecGripType == ESecondaryGripType::SG_None)
	{
		return false;
	}

	return true;
}

bool UPhysicsHandComponent::TryGrab(UObject* ObjectToGrip, const bool bIsSlotGrip, const FTransform GripTransform, const FName OptionalBoneName, const FName SlotName, const bool bIsSecondaryGrip, const FGameplayTag GripSecondaryTag)
{
	// normalise rotation if necessary
	FTransform NormalizedTransform = GripTransform; 
	if(!NormalizedTransform.IsRotationNormalized()) NormalizedTransform.NormalizeRotation();

	// GrippedComponent = Cast<UWobHandSocketComponent>(UHandSocketComponent::GetHandSocketComponentFromObject(ObjectToGrip, SlotName));

	bool bImplementsInterface = ObjectToGrip->Implements<UVRGripInterface>();

	// abort if we're already holding it in this hand.
	if (ThisHand->GetIsObjectHeld(ObjectToGrip)) return false;

	bool bAllowsMultiGrip = IVRGripInterface::Execute_AllowsMultipleGrips(ObjectToGrip);

	// if our other hand is holding
	if (OtherHand->GetIsObjectHeld(ObjectToGrip))
	{
		// if we can't multigrip
		if (!bAllowsMultiGrip)
		{
			// try to take it from the other hand if primary grip
			if (bIsSlotGrip && !bIsSecondaryGrip)
			{
				OtherHand->DropObject(ObjectToGrip);

				return ThisHand->GripObjectByInterface(ObjectToGrip, NormalizedTransform, true, OptionalBoneName, SlotName, bIsSlotGrip);
			}

			// otherwise try secondary grip. 
			if (TrySecondaryGripObject(ObjectToGrip, NormalizedTransform, SlotName, bIsSlotGrip, GripSecondaryTag))
			{
				return true;
			}
			// if secondary grip failed, drop object from other hand 
			OtherHand->DropObject(ObjectToGrip);
		}
		// grip with this hand.
		if(bImplementsInterface) return ThisHand->GripObjectByInterface(ObjectToGrip, NormalizedTransform, true, OptionalBoneName, SlotName, bIsSlotGrip);

		return ThisHand->GripObject(
			ObjectToGrip, 
			GripTransform, 
			true, 
			SlotName, 
			OptionalBoneName, 
			EGripCollisionType::InteractiveCollisionWithPhysics,
			EGripLateUpdateSettings::NotWhenCollidingOrDoubleGripping, 
			EGripMovementReplicationSettings::ForceClientSideMovement, 
			2250.f, 
			140.f, 
			bIsSlotGrip
		);
	}

	// Fail if trying to secondary grip, because our other hand cannot be holding it at this point. 
	if (bIsSecondaryGrip) return false;

	// if the item is already held by something and isnt allowing multigrip then fail.
	if (bImplementsInterface)
	{
		TArray<FBPGripPair> HoldingControllers;
		bool bIsHeld;
		IVRGripInterface::Execute_IsHeld(ObjectToGrip, HoldingControllers, bIsHeld);
		if (bIsHeld && !bAllowsMultiGrip) return false;

		// try to grip.
		return ThisHand->GripObjectByInterface(ObjectToGrip, NormalizedTransform, true, OptionalBoneName, SlotName, bIsSlotGrip);
	}


	return ThisHand->GripObject(
		ObjectToGrip,
		NormalizedTransform,
		true,
		SlotName,
		OptionalBoneName,
		EGripCollisionType::InteractiveCollisionWithPhysics,
		EGripLateUpdateSettings::NotWhenCollidingOrDoubleGripping,
		EGripMovementReplicationSettings::ForceClientSideMovement,
		2250.f,
		140.f,
		bIsSlotGrip
	);
}

void UPhysicsHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPhysicsHandComponent::OnDropped(const FBPActorGripInformation& GripInformation, bool bWasSocketed)
{
	 DetachFromObject(GripInformation);
}

void UPhysicsHandComponent::OnGrabbed(const FBPActorGripInformation& GripInformation)
{
	 InitGrip(GripInformation);
}

void UPhysicsHandComponent::OnSecondaryGripAdded(const FBPActorGripInformation& GripInformation)
{
	 OtherHandComponent->OnSecondaryGripAddedOnOther(GripInformation);
	 //OtherHand->OnGrippedObject.Broadcast(GripInformation);
}

void UPhysicsHandComponent::OnSecondaryGripRemoved(const FBPActorGripInformation& GripInformation)
{

	OtherHand->OnDroppedObject.Broadcast(GripInformation, true);
}

bool UPhysicsHandComponent::ValidateGameplayTagContainer(const FGameplayTag BaseTag, UObject* Object, const FGameplayTag DefaultTag, const FGameplayTagContainer GameplayTags) const
{
	IGameplayTagAssetInterface* ObjAsInterface = Cast<IGameplayTagAssetInterface>(Object);
	
	if (!ObjAsInterface) return false;

	// if missing match, fall back to default tag
	if (!ObjAsInterface->HasMatchingGameplayTag(BaseTag))
	{
		return GameplayTags.HasTagExact(DefaultTag);
	}
	FGameplayTagContainer OwnedTags;
	ObjAsInterface->GetOwnedGameplayTags(OwnedTags);
	
	return UVRExpansionFunctionLibrary::MatchesAnyTagsWithDirectParentTag(BaseTag, GameplayTags,OwnedTags);
}

bool UPhysicsHandComponent::ValidateGameplayTag(const FGameplayTag BaseTag, const FGameplayTag GameplayTag, UObject* Object, const FGameplayTag DefaultTag) const
{
	IGameplayTagAssetInterface* AsGTA = Cast<IGameplayTagAssetInterface>(Object);

	if (!AsGTA) return false;

	if (!GameplayTag.IsValid() || !AsGTA->HasMatchingGameplayTag(BaseTag))
	{
		return GameplayTag == DefaultTag;
	}

	return AsGTA->HasMatchingGameplayTag(GameplayTag);

}

FTransform UPhysicsHandComponent::MirrorRelativeTransform(const FTransform& OriginalTransform)
{
	// Get the original translation, rotation, and scale
	FVector OriginalTranslation, OriginalScale;
	FQuat OriginalRotation;
	OriginalTranslation = OriginalTransform.GetLocation();
	OriginalScale = OriginalTransform.GetScale3D();
	OriginalRotation = OriginalTransform.GetRotation();

	// Mirror the translation along the X-axis
	FVector MirroredTranslation = FVector(-OriginalTranslation.X, OriginalTranslation.Y, OriginalTranslation.Z);

	// Mirror the rotation by negating the roll
	FQuat MirroredRotation = FQuat(-OriginalRotation.X, -OriginalRotation.Y, OriginalRotation.Y, OriginalRotation.W);

	// Keep the original scale
	FVector MirroredScale = OriginalScale;

	// Create a mirrored transform
	FTransform MirroredTransform(MirroredRotation, MirroredTranslation, MirroredScale);

	return MirroredTransform;
}

FTransform UPhysicsHandComponent::GetBoneTransform(UObject* Object, FName BoneName)
{
	if (BoneName == NAME_None) return FTransform();

	USceneComponent* ObjAsSC = Cast<USceneComponent>(Object);
	if (ObjAsSC)
	{
		return ObjAsSC->GetSocketTransform(BoneName);
	}

	ASkeletalMeshActor* ObjAsSMA = Cast<ASkeletalMeshActor>(Object);
	if (ObjAsSMA)
	{
		return ObjAsSMA->GetSkeletalMeshComponent()->GetSocketTransform(BoneName);
	}

	return FTransform();
}

void UPhysicsHandComponent::RemoveControllerScale(FTransform& SocketTransform)
{
	FVector Scale = ThisHand->GetPivotTransform().GetScale3D() / FVector(1);
	FTransform Pivot; Pivot.SetScale3D(Scale);
	SocketTransform = UKismetMathLibrary::ComposeTransforms(SocketTransform, Pivot);
}

bool UPhysicsHandComponent::TrySecondaryGripObject(UObject* ObjectToGrab, const FTransform RelativeTransform, const FName SlotName, const bool bHadSlot, const FGameplayTag GripSecondaryTag)
{
	if (!ObjectToGrab->Implements<UVRGripInterface>()) return false;
	
	ESecondaryGripType SGripType = IVRGripInterface::Execute_SecondaryGripType(ObjectToGrab);

	if(!CanSecondaryGripObject(ObjectToGrab, bHadSlot, SGripType, GripSecondaryTag)) return false;
	
	// add secondary attach point
	return OtherHand->AddSecondaryAttachmentPoint(ObjectToGrab, ThisHand, RelativeTransform, true, 0.25f, bHadSlot, SlotName);

}

bool UPhysicsHandComponent::GetGripComponentInfo(UPrimitiveComponent* GripComponent, UObject*& OutGripObject, FTransform& WorldTransform, bool& bImplementsInterface)
{
	if (GripComponent->Implements<UVRGripInterface>()) 
	{
		bImplementsInterface = true;
		WorldTransform = GripComponent->GetComponentTransform();
		OutGripObject = GripComponent;
		return true;
	}
	
	AActor* Owner = GripComponent->GetOwner();
	if (Owner && Owner->Implements<UVRGripInterface>())
	{
		bImplementsInterface = true;
		WorldTransform = Owner->GetActorTransform();
		OutGripObject = Owner;
		return true;
	}

	return false;
}

void UPhysicsHandComponent::GetObjectThrowVelocity(FBPActorGripInformation& GripInfo, const FVector AngularVelocity, const FVector ObjectsLinearVelocity, FVector& OutAngularVelocity, FVector& OutLinearVelocity)
{

	if (GripInfo.GrippedObject->Implements<UVRGripInterface>())
	{
		TArray<FBPGripPair> Grippers;
		bool bIsHeld;
		IVRGripInterface::Execute_IsHeld(GripInfo.GrippedObject, Grippers, bIsHeld);

		// exit if there's more than one hand holding.
		if (Grippers.Num() > 1) return;

		// @TODO: Add optional sample grip velocity option here.

		OutAngularVelocity = AngularVelocity;
		FVector LocalVelocity = bUseControllerVelocityOnRelease ? ThisHand->GetComponentVelocity() : ObjectsLinearVelocity;

		// calculate limited velocity if specified and return if reduced.
		if (bLimitMaxThrowVelocity)
		{
			if (UKismetMathLibrary::VSizeSquared(LocalVelocity) > FMath::Pow(MaxThrowVelocity, 2))
			{
				FVector NormalizedLV = LocalVelocity; UKismetMathLibrary::Vector_Normalize(NormalizedLV);
				OutLinearVelocity = NormalizedLV * MaxThrowVelocity;
				return;
			}
		}

		// return full velocity.
		OutLinearVelocity = LocalVelocity;
		return;
	}


}

bool UPhysicsHandComponent::CheckIsValidForGripping(UObject* Object, const FGameplayTagContainer RelevantTags)
{
	ESecondaryGripType GripType;
	if (CanAttemptSecondaryGrabOnObject(Object, GripType))
	{
		FGameplayTag BaseTag = FGameplayTag::RequestGameplayTag(FName("GripType.Secondary"));
		FGameplayTag DefaultSecondaryTag = FGameplayTag::RequestGameplayTag(FName("GripType.Secondary.OnPrimaryGrip"));
		if (ValidateGameplayTagContainer(BaseTag, Object, DefaultSecondaryTag, RelevantTags))
		{
			return true;
		}
	}
		
	FGameplayTag BaseTag = FGameplayTag::RequestGameplayTag(FName("GripType"));
	FGameplayTag DefaultGripTag = FGameplayTag::RequestGameplayTag(FName("GripType.OnPrimaryGrip"));

	if (ValidateGameplayTagContainer(BaseTag, Object, DefaultGripTag, RelevantTags))
	{
		return true;
	}
	
	return false;
}

FName UPhysicsHandComponent::GetCorrectPrimarySlotPrefix(UObject* ObjectToCheckForTag, const EControllerHand Hand, const FName NearestBoneName)
{
	if (!ObjectToCheckForTag) return NAME_None;

	FString LocalPrefix = NearestBoneName != NAME_None ? NearestBoneName.ToString() : "";

	if (Cast<IGameplayTagAssetInterface>(ObjectToCheckForTag)->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("GripSockets.SeperateHandSockets"))))
	{
		switch (Hand)
		{
			case(EControllerHand::Left):
				return FName(LocalPrefix.Append("VRGripLP"));

			case(EControllerHand::Right):
				return FName(LocalPrefix.Append("VRGripRP"));

			case(EControllerHand::AnyHand):
				return FName(LocalPrefix.Append("VRGripP"));

			case(EControllerHand::Pad):
				return FName(LocalPrefix.Append("VRGripP"));

			case(EControllerHand::ExternalCamera):
				return FName(LocalPrefix.Append("VRGripP"));

			case(EControllerHand::Gun):
				return FName(LocalPrefix.Append("VRGripP"));

			default:
				return FName();
		}
	}
	
	return FName();
}

void UPhysicsHandComponent::InitGrip(const FBPActorGripInformation GripInfo)
{
	IGameplayTagAssetInterface* AsTagInterface = Cast<IGameplayTagAssetInterface>(GripInfo.GrippedObject);
	if (!AsTagInterface) return;

	bool bDoNotAttach = AsTagInterface->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("GripSockets.DontAttachHand")));

	if (bAlreadyGrasped && !bIsSecondaryGripping || bDoNotAttach || GripInfo.bIsLerping) return;

	bAlreadyGrasped = true;
	bIsSecondaryGripping = false;

	RetrievePoses(GripInfo);
	InitialiseAndAttach(GripInfo, bIsSecondaryGripping, bHasCustomAnimation);

}

void UPhysicsHandComponent::RetrievePoses(const FBPActorGripInformation GripInfo)
{
	bool bCondition = bIsSecondaryGripping ? GripInfo.SecondaryGripInfo.bIsSlotGrip : GripInfo.bIsSlotGrip;

	if (bCondition)
	{
		FName SocketName = bIsSecondaryGripping ? GripInfo.SecondaryGripInfo.SecondarySlotName : GripInfo.SlotName;
		UHandSocketComponent* HandSocket = UHandSocketComponent::GetHandSocketComponentFromObject(GripInfo.GrippedObject, SocketName);

		if (HandSocket)
		{
			if (HandSocket->bOnlyUseHandPose)
			{
				bUseTargetMeshTransform = false;
			}

			if (!HandSocket->bOnlyUseHandPose)
			{
				TargetMeshTransform = HandSocket->GetMeshRelativeTransform(bIsRightHand, false, true);
				bUseTargetMeshTransform = true;
			}

			if (HandSocket->GetBlendedPoseSnapShot(CustomSnapshot, HandMeshComponent))
			{
				bHasCustomAnimation = true;
				bCustomAnimIsSnapshot = true;
				SetHandGripAnimation(HandSocket->GetTargetAnimation());
				return;
			}
		}
		 SetHandGripAnimation(HandSocket->HandTargetAnimation);
		return;
	}
	bUseTargetMeshTransform = false;
	bHasCustomAnimation = false;
	bCustomAnimIsSnapshot = false;
	CustomSnapshot = FPoseSnapshot();
	return;
}

bool UPhysicsHandComponent::InitialiseAndAttach(const FBPActorGripInformation GripInfo, const bool bSecondaryGrip, const bool bSkipEvaluation)
{


	// get the appropriate grip transform, inverse if secondary
	FTransform SecondaryRelative = (FTransform) GripInfo.SecondaryGripInfo.SecondaryRelativeTransform;
	SecondaryRelative = SecondaryRelative.Inverse();
	FTransform PrimaryRelative = GripInfo.RelativeTransform;
	FTransform GripRelativeTransform = bSecondaryGrip ? SecondaryRelative : PrimaryRelative;
	FTransform FinalRelativeNonTarget = UKismetMathLibrary::MakeRelativeTransform(MeshBaseTransform * ThisHand->GetComponentTransform(), GripRelativeTransform * ThisHand->GetPivotTransform());
//	FTransform FinalAttachTransform = bUseTargetMeshTransform ? TargetMeshTransform : FinalRelativeNonTarget;


	UHandSocketComponent* SocketComponent = UHandSocketComponent::GetHandSocketComponentFromObject(GripInfo.GrippedObject, bSecondaryGrip ? GripInfo.SecondaryGripInfo.SecondarySlotName : GripInfo.SlotName);
	if (SocketComponent)
	{
		HandMeshComponent->AttachToComponent(SocketComponent->GetAttachParent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		// FTransform SocketMeshTransform = SocketComponent->GetMeshRelativeTransform(bIsRightHand, true, true);

		if (bUseTargetMeshTransform)
		{
			HandMeshComponent->SetRelativeLocationAndRotation(TargetMeshTransform.GetLocation(), TargetMeshTransform.GetRotation());
			return true;
		}
	}

	if (GripInfo.GripTargetType == EGripTargetType::ActorGrip)
	{
		AActor* GrippedActor = Cast<AActor>(GripInfo.GrippedObject);
		if (GrippedActor)
		{

			HandMeshComponent->AttachToComponent(GrippedActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform, GripInfo.GrippedBoneName);
			return true;
		}
	}

	if (GripInfo.GripTargetType == EGripTargetType::ComponentGrip)
	{
		UPrimitiveComponent* GrippedComp = Cast<UPrimitiveComponent>(GripInfo.GrippedObject);
		if (GrippedComp)
		{
			HandMeshComponent->AttachToComponent(GrippedComp, FAttachmentTransformRules::KeepWorldTransform, GripInfo.GrippedBoneName);
			return true;
		}
	}

	if (bUseTargetMeshTransform)
	{
		// HandMeshComponent->SetRelativeLocationAndRotation(TargetMeshTransform.GetLocation(), TargetMeshTransform.GetRotation());
		return true;
	}

	HandMeshComponent->SetRelativeLocationAndRotation(GripRelativeTransform.GetLocation(), GripRelativeTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	
	return true;
}

void UPhysicsHandComponent::SetHandGripAnimation(UAnimSequence* AnimationSequence)
{
	// just set the reference if we're using the animBP
	HandAnimSequence = AnimationSequence;

	if (bUseAnimBlueprint)
	{
		return;
	}

	// directly set the animation to play if not using animBP.
	if (AnimationSequence)
	{
		HandMeshComponent->PlayAnimation(AnimationSequence, true);
		return;
	}

	// if the anim is invalid then stop animating.
	HandMeshComponent->PlayAnimation(DefaultAnimation, true);
}

bool UPhysicsHandComponent::DetachFromObject(const FBPActorGripInformation GripInfo)
{
	bAlreadyGrasped = false;
	HandMeshComponent->AttachToComponent(ThisHand, FAttachmentTransformRules::KeepRelativeTransform);
	HandMeshComponent->SetRelativeTransform(MeshBaseTransform);
	HandAnimSequence = DefaultAnimation;

	return true;
}

bool UPhysicsHandComponent::TryClimb(UObject* TargetObject, const bool bCanCheckClimb)
{
	if (bCanCheckClimb)
	{

		UE_LOG(LogTemp, Warning, TEXT("Checking climb"));
		bool bIsObjectRelative = TargetObject->IsValidLowLevel() ? true : false;

		UE_LOG(LogTemp, Warning, TEXT("object is %s"), *TargetObject->GetName());
		USceneComponent* AsComponent = Cast<USceneComponent>(TargetObject);
		
		UHandSocketComponent* SocketComponent = UHandSocketComponent::GetHandSocketComponentFromObject(TargetObject, FName("ClimbSocket"));

		if (!SocketComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("failed to get socket from object %s"), *TargetObject->GetName());
			return false;
		}

		FTransform GripLocation;

		if (!AsComponent)
		{
			AsComponent = Cast<USceneComponent>(Cast<AActor>(TargetObject)->GetRootComponent());
		}

		if (!AsComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failde to get root component"));
			return false;
		}
		if (AsComponent)
		{
			GripLocation = UKismetMathLibrary::MakeRelativeTransform(ThisHand->GetPivotTransform(), AsComponent->GetComponentTransform());
		}

		UE_LOG(LogTemp, Warning, TEXT("initialising climb"));
		bIsClimbing = InitialiseAndAttachClimbing(TargetObject, GripLocation);

		if (bIsClimbing)
		{
			OnStartClimbing.Broadcast(bIsRightHand, TargetObject);
		}

		return bIsClimbing;
	}
	return false;
}

bool UPhysicsHandComponent::InitialiseAndAttachClimbing(UObject* GrippedObject, const FTransform RelativeTranform)
{
	if (!GrippedObject) return false;

	// for the sake of consistency and explicit declaration of climbable objects, only hand socket components are climbable. 
	// If we somehow get here and the object doesn't have a hand socket, climbing will fail. 
	
	UHandSocketComponent* SocketComponent = UHandSocketComponent::GetHandSocketComponentFromObject(GrippedObject, FName("ClimbSocket"));

	if (SocketComponent)
	{
		if (SocketComponent->bOnlyUseHandPose)
		{
				bUseTargetMeshTransform = false;
		}

		if (!SocketComponent->bOnlyUseHandPose)
		{
			TargetMeshTransform = SocketComponent->GetMeshRelativeTransform(bIsRightHand, false, true);
			bUseTargetMeshTransform = true;
		}
		
		if (SocketComponent->GetBlendedPoseSnapShot(CustomSnapshot, HandMeshComponent))
		{
			bHasCustomAnimation = true;
			bCustomAnimIsSnapshot = true;
			SetHandGripAnimation(SocketComponent->GetTargetAnimation());
		}		
		SetHandGripAnimation(SocketComponent->HandTargetAnimation);

		HandMeshComponent->AttachToComponent(SocketComponent->GetAttachParent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		if (bUseTargetMeshTransform)
		{
			HandMeshComponent->SetRelativeLocationAndRotation(TargetMeshTransform.GetLocation(), TargetMeshTransform.GetRotation());
		}
		return true;
	}

	bUseTargetMeshTransform = false;
	bHasCustomAnimation = false;
	bCustomAnimIsSnapshot = false;
	CustomSnapshot = FPoseSnapshot();
	return false;
}


#undef PRINT