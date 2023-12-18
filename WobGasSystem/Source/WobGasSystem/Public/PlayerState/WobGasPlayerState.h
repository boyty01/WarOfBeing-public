// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "WobGasPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class WOBGASSYSTEM_API AWobGasPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
	AWobGasPlayerState();

public: 

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void InitialiseAttributes();

	virtual void GiveAbilities();

	virtual void PostInitializeComponents() override;

	virtual void ClientInitialize(AController* C) override;
protected:

	// Define the replication mode for gameplay effects. Defaults to full. Useful for limiting network traffic where applicable in multiplayer games.
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Replication")
	EGameplayEffectReplicationMode ReplicationMode{ EGameplayEffectReplicationMode::Full };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWOBAbilityComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<class UWobGasAttributes> Attributes;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultAttributeEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<class UWobGasAbilityBase>> DefaultAbilities;
};
