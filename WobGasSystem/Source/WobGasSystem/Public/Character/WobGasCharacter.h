// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interface/WobGasInterface.h"
#include "WobGasCharacter.generated.h"

UCLASS()
class WOBGASSYSTEM_API AWobGasCharacter : public ACharacter, public IAbilitySystemInterface, public IWobGasInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AWobGasCharacter();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void InitialiseAttributes();

	virtual void GiveAbilities();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	/* WobGasInterface */

	UFUNCTION(BlueprintImplementableEvent, Category = "Attribute Events")
	void OnDeath();


protected:
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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	// Binds any events to the attribute event delegates defined in WOBAbilityComponent
	virtual void BindAttributeListeners() const;
};
