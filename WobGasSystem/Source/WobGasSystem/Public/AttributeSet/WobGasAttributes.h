// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WobGasAttributes.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)\
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)\
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)\
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)\
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class WOBGASSYSTEM_API UWobGasAttributes : public UAttributeSet
{

	GENERATED_BODY()

public:

	UWobGasAttributes();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data);



	/* ---------------- ATTRIBUTES -------------*/

	/* ----- Dynamic Effect Damage Magnitude Specifier ----- */

	// This isn't a specific attribute. it exists so that gameplay effects can specify a base damage
	// that executions can draw on to calculate the damage from a small set of damage calculations. Though every 
	// ability system will have one, its value can be largely ignored. 
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_EffectDamageMagSpecifier)
	FGameplayAttributeData EffectDamageMagSpecifier;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, EffectDamageMagSpecifier);

	UFUNCTION()
	virtual void OnRep_EffectDamageMagSpecifier(const FGameplayAttributeData& OldEffectDamageSpecifier);


	/* ---- HEALTH ---- */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, Health);

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, MaxHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldHealth);


	/* Level and Experience */

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CharacterLevel)
	FGameplayAttributeData CharacterLevel;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, CharacterLevel);

	UFUNCTION()
	virtual void OnRep_CharacterLevel(const FGameplayAttributeData& OldCharacterLevel);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ExperiencePoints)
	FGameplayAttributeData ExperiencePoints;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, ExperiencePoints);

	UFUNCTION()
	virtual void OnRep_ExperiencePoints(const FGameplayAttributeData& OldExperiencePoints);

	// This is for NPC's to be able to set a base experience reward when killed. Not necessary for the player.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_BaseExperienceWorth)
	FGameplayAttributeData BaseExperienceWorth;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, BaseExperienceWorth);

	UFUNCTION()
	virtual void OnRep_BaseExperienceWorth(const FGameplayAttributeData& OldBaseExperienceWorth);


	/* ---- RESISTANCES  ---- */

	/*
	* Resistances should be >= 0 and < MaxResistance. to account for a minimum of damage passthrough any resistance, this value should be less than 1.0.
	* If you want to be immune to something, you should apply an appropriate immunity gameplay tag.
	* Resistance values will be clamped to MaxResistance in post gameplay effect
	*/

	// This is the default resistance cap. In most cases it's best not to adjust this per-instance, but is exposed for edge cases.
	UPROPERTY(BlueprintReadWrite, Category = "Attributes")
	float MaxResistance{ 0.95f };

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_PhysicalResistance)
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UWobGasAttributes, PhysicalResistance);

	UFUNCTION()
	virtual void OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance);

private :
	void ClampAttributeValues();
};
