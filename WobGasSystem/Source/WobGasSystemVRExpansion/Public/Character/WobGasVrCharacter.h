// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "AbilitySystemInterface.h"
#include "Interface/WobGasInterface.h"
#include "InputCoreTypes.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "WobGasVrCharacter.generated.h"

/**
 * 
 */
UCLASS()
class WOBGASSYSTEMVREXPANSION_API AWobGasVrCharacter : public AVRCharacter, public IAbilitySystemInterface, public IWobGasInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AWobGasVrCharacter();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void InitialiseAttributes();

	virtual void GiveAbilities();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	/* WobGasInterface */

	UFUNCTION(BlueprintImplementableEvent, Category = "Attribute Events")
	void OnDeath();

	UFUNCTION(BlueprintCallable, Category = "Animation Data")
	TEnumAsByte<enum EControllerHand> GetAnimOwningControllerHand(UAnimInstance* TargetAnimInstance);

	UFUNCTION(BlueprintCallable, Category = "Animation Data")
	UAnimSequence* GetCurrentAnimationSequence(const EControllerHand Hand);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category ="Climbing")
	bool IsClimbing() { return bIsClimbing; };

	UFUNCTION(BlueprintPure, Blueprintcallable, Category = "Climbing")
	UGripMotionControllerComponent* GetClimbingController() { return ClimbingController; };

protected:

	bool bIsClimbing;

	TObjectPtr<UGripMotionControllerComponent> ClimbingController;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWOBAbilityComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<class UWobGasAttributes> Attributes;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultAttributeEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<class UWobGasAbilityBase>> DefaultAbilities;


	/* ----- VR HANDS ----- */

	/*Handles tracing from the specified grip component and tries to interact with any grippable components and objects. returns true if something gripped.  */
	UFUNCTION(BlueprintCallable, Category = "VR Grip")
	bool TraceAndTryGrip(UGripMotionControllerComponent* GripComponent);

	// Max trace distance
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VR Grip")
	float GripTraceDistance{ 1.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component")
	TObjectPtr<class UPhysicsHandComponent> RightHandPhysicsComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Hand Components")
	TObjectPtr<USkeletalMeshComponent> RightHandMesh;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Hand Components")
	TObjectPtr<class USphereComponent> RightHandPhysRoot;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Hand Components")
	TObjectPtr<USceneComponent> RightAimComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component")
	TObjectPtr<class UPhysicsHandComponent> LeftHandPhysicsComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Hand Components")
	TObjectPtr<USkeletalMeshComponent> LeftHandMesh;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Hand Components")
	TObjectPtr<class USphereComponent> LeftHandPhysRoot;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Hand Components")
	TObjectPtr<USceneComponent> LeftAimComponent;

	// enable debug trace lines for grip trace. editor only.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VR Grip")
	bool bDebugTrace{ false };


	FRotator RightHandLocalRotOffset;

	FRotator LeftHandLocalRotOffset;

	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void VRMoveForward(const float val);

	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void VRStrafeRight(const float val);

	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void VRTurnRight(const float val);

	UFUNCTION()
	void OnHandStartedClimb(bool& bIsRightHand, UObject* ClimbObject);

	UFUNCTION()
	void OnHandEndedClimb(bool& bIsRightHand);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	// Object that is being grabbed
	TObjectPtr<UObject> GrabObject;

	// Attempts ot find the nearest grabbable object in range. 
	bool FindGrabObject();

	// Binds any events to the attribute event delegates defined in WOBAbilityComponent
	virtual void BindAttributeListeners() const;


};
