// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WOBAbilityComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamageTaken, float&, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZeroHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGainedExperience, float&, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelUp);
/**
 * 
 */
UCLASS()
class WOBGASSYSTEM_API UWOBAbilityComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public :

	void GetNonConstAttributes();

	/* ---- DELEGATES ----
 These are triggered via PostGameplayEffectExecute when appropriate scenarios are evaluated.
 Used to broadcast certain events off the back of executions. These are used to effect gameplay
 when certain attributes are modified or hit certain conditions, such as health reaching zero.
 It's also possible to build combat logs out of data captured from executions if desired.
*/
	UPROPERTY(BlueprintAssignable)
	FDamageTaken OnDamageTaken;

	UPROPERTY(BlueprintAssignable)
	FZeroHealth OnZeroHealth;

	UPROPERTY(BlueprintAssignable)
	FOnGainedExperience OnGainExperience;

	UPROPERTY(BlueprintAssignable)
	FOnLevelUp OnLevelUp;

};
