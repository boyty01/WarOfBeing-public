// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEC_ApplyDamage.generated.h"

/**
 * 
 */
UCLASS()
class WOBGASSYSTEM_API UGEC_ApplyDamage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

	UGEC_ApplyDamage();
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	FGameplayEffectAttributeCaptureDefinition TargetHealthDef;
	FGameplayEffectAttributeCaptureDefinition TargetPhysicalResistanceDef;
};
