// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "WobGasCharacterExample.generated.h"

UCLASS()
class WOBGASSYSTEM_API AWobGasCharacterExample : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AWobGasCharacterExample();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability System")
	UAbilitySystemComponent* K2_GetAbilitySystemComponent() const;
	UAbilitySystemComponent* K2_GetAbilitySystemComponent_Implementation() const;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
