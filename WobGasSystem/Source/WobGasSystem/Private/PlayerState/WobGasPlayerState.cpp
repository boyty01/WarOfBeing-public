// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/WobGasPlayerState.h"
#include "ActorComponent/WOBAbilityComponent.h"
#include "Ability/WobGasAbilityBase.h"

AWobGasPlayerState::AWobGasPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UWOBAbilityComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(ReplicationMode);

	Attributes = CreateDefaultSubobject<UWobGasAttributes>(TEXT("Attributes"));
}

UAbilitySystemComponent* AWobGasPlayerState::GetAbilitySystemComponent() const
{
	return Cast<UAbilitySystemComponent>(AbilitySystemComponent.Get());
}

void AWobGasPlayerState::InitialiseAttributes()
{
	if (AbilitySystemComponent && DefaultAttributeEffect)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1, EffectContext);

		if (SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle EffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void AWobGasPlayerState::GiveAbilities()
{
	if (HasAuthority() && AbilitySystemComponent)
	{
		for (TSubclassOf<UWobGasAbilityBase>& StartupAbility : DefaultAbilities)
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(StartupAbility, 1, static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputId), this));
		}
	}
}

void AWobGasPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

		InitialiseAttributes();
		GiveAbilities();
	}
}

void AWobGasPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	InitialiseAttributes();
}
