// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grippables/HandSocketComponent.h"
#include "WobGripHandSocketComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class WOBGASSYSTEMVREXPANSION_API UWobGripHandSocketComponent : public UHandSocketComponent
{
	GENERATED_BODY()
	
	UWobGripHandSocketComponent(const FObjectInitializer& ObjectInitializer);
};
