// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HandComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WOBGASSYSTEMVREXPANSION_API UHandComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHandComponent();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
	TObjectPtr<class USphereComponent> PhysicsRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
	TObjectPtr<class UCapsuleComponent> GrabComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UVREPhysicsConstraintComponent* PhysicsConstraint;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Fingers")
	class UVREPhysicalAnimationComponent* PhysicalAnimation;

	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category ="Hit Events")
	void OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditDefaultsOnly, Category = "Configurations")
	FName RootBone;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
