// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSet/WobGasAttributes.h"
#include "Net/UnrealNetwork.h"
#include "ActorComponent/WOBAbilityComponent.h"
#include "GameplayEffectExtension.h"

UWobGasAttributes::UWobGasAttributes()
{
}

void UWobGasAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME_CONDITION_NOTIFY(UWobGasAttributes, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWobGasAttributes, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWobGasAttributes, EffectDamageMagSpecifier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWobGasAttributes, CharacterLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWobGasAttributes, ExperiencePoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWobGasAttributes, BaseExperienceWorth, COND_None, REPNOTIFY_Always);

}

void UWobGasAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{

	// Clamp all attributes 
	ClampAttributeValues();


	UWOBAbilityComponent* ASC = Cast<UWOBAbilityComponent>(GetOwningAbilitySystemComponent());

	// Health mod events
	if (Data.EvaluatedData.Attribute.GetName().Equals("Health"))
	{
		ASC->OnDamageTaken.Broadcast(Data.EvaluatedData.Magnitude);
		// check if character should be dead. Broadcast if so.
		if (Health.GetCurrentValue() <= 0.f)
		{
			if (ASC)
			{
				ASC->OnZeroHealth.Broadcast();
			}
		}
	}

	// Experience gains
	if (Data.EvaluatedData.Attribute.GetName().Equals("BaseExperienceWorth"))
	{
		ASC->OnGainExperience.Broadcast(Data.EvaluatedData.Magnitude);
	}

	//Level up
	if (Data.EvaluatedData.Attribute.GetName().Equals("CharacterLevel"))
	{
		ASC->OnLevelUp.Broadcast();
	}
}



void UWobGasAttributes::OnRep_EffectDamageMagSpecifier(const FGameplayAttributeData& OldEffectDamageSpecifier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, EffectDamageMagSpecifier, OldEffectDamageSpecifier);
}

void UWobGasAttributes::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, Health, OldHealth);
}

void UWobGasAttributes::OnRep_MaxHealth(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, MaxHealth, OldHealth);
}

void UWobGasAttributes::OnRep_CharacterLevel(const FGameplayAttributeData& OldCharacterLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, CharacterLevel, OldCharacterLevel);
}

void UWobGasAttributes::OnRep_ExperiencePoints(const FGameplayAttributeData& OldExperiencePoints)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, ExperiencePoints, OldExperiencePoints);
}

void UWobGasAttributes::OnRep_BaseExperienceWorth(const FGameplayAttributeData& OldBaseExperienceWorth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, BaseExperienceWorth, OldBaseExperienceWorth);
	
}

void UWobGasAttributes::OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWobGasAttributes, PhysicalResistance, OldPhysicalResistance);
}

void UWobGasAttributes::ClampAttributeValues()
{
	// Clamp Resistances
	PhysicalResistance.SetCurrentValue(FMath::Clamp(PhysicalResistance.GetCurrentValue(), 0.f, MaxResistance));


	// Clamp health 
	Health.SetCurrentValue(FMath::Clamp(Health.GetCurrentValue(), 0.f, MaxHealth.GetCurrentValue()));
}
