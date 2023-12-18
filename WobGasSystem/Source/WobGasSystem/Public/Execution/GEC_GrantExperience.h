// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEC_GrantExperience.generated.h"

/**
 * 
 */
UCLASS()
class WOBGASSYSTEM_API UGEC_GrantExperience : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	FGameplayEffectAttributeCaptureDefinition ExperiencePointsDef;
	FGameplayEffectAttributeCaptureDefinition CharacterLevelReceiverDef;
	FGameplayEffectAttributeCaptureDefinition CharacterLevelGiverDef;
	FGameplayEffectAttributeCaptureDefinition BaseExperienceWorthDef;


};
