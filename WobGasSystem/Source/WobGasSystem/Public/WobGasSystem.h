// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

UENUM(BlueprintType)
enum class EGasAbilityInputId : uint8
{
	None,
	Confirm,
	Cancel
};


class FWobGasSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
