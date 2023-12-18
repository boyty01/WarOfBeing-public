// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRBPDataTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagsClasses.h"
#include "PhysicsHandComponent.generated.h"

UENUM(BlueprintType)
enum EVrHandDesignate
{
	RightHand,
	LeftHand
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHandStartClimbing, bool &, bRightHand, UObject*, GrippedObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandEndClimbing, bool &, bRightHand);


/*Actor Component that handles VR Hand logic. Requires manually setting references to appropriate components that have a specific hierarchy. 
see C++ for info. See WobGasVrCharacter for expected hierarchy of components attached to a motion controller component. Currently does not support physical anims,
despite the name. */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WOBGASSYSTEMVREXPANSION_API UPhysicsHandComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPhysicsHandComponent();


	// Climbing Delegates
	UPROPERTY(BlueprintAssignable, Category = "Climbing")
	FOnHandStartClimbing OnStartClimbing;

	UPROPERTY(BlueprintAssignable, Category = "Climbing")
	FOnHandEndClimbing OnEndClimbing;
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Climbing")
	bool IsClimbing() { return bIsClimbing; };

	// entry point for trying to grip or drop a grippable. 
	UFUNCTION(BlueprintCallable, Category = "Grip Events")
	bool TriggerGripOrDrop(const bool bIsGrip);

	UFUNCTION(BlueprintCallable, Category = "Init")
	void InitHandComponent(UGripMotionControllerComponent* CComp, UGripMotionControllerComponent* OtherCComp, UPrimitiveComponent* TraceComp, USceneComponent* AimComp, USkeletalMeshComponent* HandMesh, UPhysicsHandComponent* _OtherHandComponent);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	UAnimSequence* GetCurrentAnimation() { return HandAnimSequence; };


	void OnSecondaryGripAddedOnOther(const FBPActorGripInformation GripInfo);

//	void OnSecondaryGripRemovedOnOther(const FBPActorGripInformation GripInfo);
protected:

	TObjectPtr<UPhysicsHandComponent> OtherHandComponent; 

	bool GripOrDropObject(const FGameplayTagContainer RelevantTags, const bool bCanCheckClimb);

	void DropItems(const FGameplayTagContainer GameplayTags);

	bool DropSecondaryAttachment(const FGameplayTagContainer GameplayTags);

	bool GetNearestOverlappingObject(UObject*& NearestObject, bool& bImplementsInterface, FTransform& ObjectTransform, bool& bCanBeClimbed, FName& BoneName, FVector& ImpactLocation, const FGameplayTagContainer RelevantTags);

	bool TraceForNearestObject(UObject*& NearestObject, bool& bImplementsInterface, FTransform& ObjectTransform, bool& bCanBeClimbed, FName& BoneName, FVector& ImpactLocation, const FGameplayTagContainer RelevantTags, UPrimitiveComponent*& FirstPrimitive);

	bool ShouldGripComponent(UPrimitiveComponent* ComponentToCheck, uint8& GripPrioToCheckAgainst, bool bCheckAgainstPrior, FName& BoneName, const FGameplayTagContainer RelevantTags, UObject*& ObjectToGrip, bool& bImplementsInterface, FTransform& ObjectWorldTransform, uint8& GripPrio);

	bool CheckGripPriority(UObject* ObjectToCheck, const bool bCheckAgainstPrior, const uint8 PrioToCheckAgainst , uint8& NewGripPrio);

	bool CheckIsClimbable(UObject* NearestObject, bool& bCanClimb);

	bool CanAttemptGrabOnObject(UObject* ObjectToCheck);

	bool CanAttemptSecondaryGrabOnObject(UObject* ObjectToCheck, ESecondaryGripType& SecGripType) const;

	bool CanSecondaryGripObject(UObject* ObjectToGrip, const bool bHadSlot, const ESecondaryGripType SecGripType, const FGameplayTag GripSecondaryTag) const;

	bool TryGrab(UObject* ObjectToGrip, const bool bIsSlotGrip, const FTransform GripTransform, const FName OptionalBoneName, const FName SlotName, const bool bIsSecondaryGrip, const FGameplayTag GripSecondaryTag = FGameplayTag::EmptyTag);
	
	// distinguish if this component represents right or left hand
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Grip Attributes")
	bool bIsRightHand;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category ="Grip Attributes")
	float GripTraceLength{ 0.1f };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Grip Attributes")
	bool bUseControllerVelocityOnRelease{ false };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Grip Attributes")
	bool bLimitMaxThrowVelocity{ false };

	// if true, trace checks for grippables will be disabled and only overlaps will be checked.
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Grip Attributes")
	bool bForceOverlapOnlyGripChecks{ false };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Grip Attributes", meta=(EditCondition="bLimitMaxThrowVelocity"))
	float MaxThrowVelocity{1000.f};

	// if true, will only store references to the animation sequence to play for the bp to query. if false, it will directly assign the animation to the hand mesh itself.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grip Animation")
	bool bUseAnimBlueprint;

	// The default animation sequence to use when not gripping.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grip Animation")
	UAnimSequence* DefaultAnimation;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; 

	UFUNCTION()
	void OnDropped(const FBPActorGripInformation& GripInformation, bool bWasSocketed);
	
	UFUNCTION()
	void OnGrabbed(const FBPActorGripInformation& GripInformation);

	UFUNCTION()
	void OnSecondaryGripAdded(const FBPActorGripInformation& GripInformation);

	UFUNCTION()
	void OnSecondaryGripRemoved(const FBPActorGripInformation& GripInformation);

private:

	bool ValidateGameplayTagContainer(const FGameplayTag BaseTag, UObject* Object, const FGameplayTag DefaultTag, const FGameplayTagContainer GameplayTags) const;

	bool ValidateGameplayTag(const FGameplayTag BaseTag, const FGameplayTag GameplayTag, UObject* Object, const FGameplayTag DefaultTag) const;

	FTransform MirrorRelativeTransform(const FTransform& OriginalTransform);

	FTransform GetBoneTransform(UObject* Object, FName BoneName);

	void RemoveControllerScale(FTransform& SocketTransform);

	bool TrySecondaryGripObject(UObject* ObjectToGrab, const FTransform RelativeTransform, const FName SlotName, const bool bHadSlot, const FGameplayTag GripSecondaryTag = FGameplayTag::EmptyTag);

	//Check a given primitive component implements the vr grip interface, if it doesn't, test its parent actor. returns a reference to the lowest level implementation.
	// or nullptr if neither do.
	bool GetGripComponentInfo(UPrimitiveComponent* GripComponent, UObject*& OutGripObject, FTransform& WorldTransform, bool& bImplementsInterface);

	void GetObjectThrowVelocity(FBPActorGripInformation& GripInfo, const FVector AngularVelocity, const FVector ObjectsLinearVelocity, FVector& OutAngularVelocity, FVector& OutLinearVelocity);

	TObjectPtr<UGripMotionControllerComponent> ThisHand;

	TObjectPtr<UGripMotionControllerComponent> OtherHand;

	// reference to the mesh component attached to the controller. 
	TObjectPtr<USkeletalMeshComponent> HandMeshComponent;

	// the component used to assess overlaps
	TObjectPtr<UPrimitiveComponent> CollisionSphere;
	
	TObjectPtr<USceneComponent> AimComponent;

	//=== Grip Snapping === 

	bool bIsGripping{ false };

	bool bShouldSnapToGrip{ false };

	// the origin transform as it is on begin play. this is the base (relative) transform of the hand when not gripping anything.
	FTransform MeshBaseTransform;

	// this is a ref to the currently gripped component, to track its transform on tick. Objects might get destroyed, so this may become invalid! Always assert.
	TObjectPtr<class UWobHandSocketComponent> GrippedComponent;

	bool CheckIsValidForGripping(UObject* Object, const FGameplayTagContainer RelevantTags);

	FName GetCorrectPrimarySlotPrefix(UObject* ObjectToCheckForTag, const EControllerHand Hand, const FName NearestBoneName);

	
	// ========= HAND GRIP / TRACKING ===========

	bool bAlreadyGrasped;

	bool bIsSecondaryGripping;

	bool bUseTargetMeshTransform;

	FTransform TargetMeshTransform;

	struct FPoseSnapshot CustomSnapshot;

	bool bHasCustomAnimation;

	bool bCustomAnimIsSnapshot;

	TObjectPtr<UAnimSequence> HandAnimSequence;

	// Called when an object is gripped. checks if the object expects the mesh to attach to it and initialises the tracking if so.
	void InitGrip(const FBPActorGripInformation GripInfo);

	void RetrievePoses(const FBPActorGripInformation GripInfo);

	bool InitialiseAndAttach(const FBPActorGripInformation GripInfo, const bool bSecondaryGrip, const bool bSkipEvaluation);

	void SetHandGripAnimation(UAnimSequence* AnimationSequence = nullptr);

	bool DetachFromObject(const FBPActorGripInformation GripInfo);

	// ==== Climbing =====

	bool bIsClimbing; 

	bool TryClimb(UObject* TargetObject, const bool bCanCheckClimb);

	bool InitialiseAndAttachClimbing(UObject* GrippedObject, const FTransform RelativeTranform);
};
