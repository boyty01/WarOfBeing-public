// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WobGasSystem.h"
#include "WobGasAbilityBase.generated.h"

/**
 * 
 */
UCLASS()
class WOBGASSYSTEM_API UWobGasAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()
	

	UWobGasAbilityBase();

public: 
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	EGasAbilityInputId AbilityInputId = EGasAbilityInputId::None;
};
