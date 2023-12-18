// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/WobGasCharacter.h"
#include "ActorComponent/WOBAbilityComponent.h"
#include "AttributeSet/WobGasAttributes.h"
#include "Ability/WobGasAbilityBase.h"

// Sets default values
AWobGasCharacter::AWobGasCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UWOBAbilityComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	Attributes = CreateDefaultSubobject<UWobGasAttributes>(TEXT("Attributes"));
}

UAbilitySystemComponent* AWobGasCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AWobGasCharacter::InitialiseAttributes()
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

void AWobGasCharacter::GiveAbilities()
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

void AWobGasCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		InitialiseAttributes();
		GiveAbilities();
	}
}

void AWobGasCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitialiseAttributes();

	if (AbilitySystemComponent && InputComponent)
	{
		const FGameplayAbilityInputBinds Binds("Confirm", "Cancel", "EGasAbilityInputId", static_cast<int32>(EGasAbilityInputId::Confirm), static_cast<int32>(EGasAbilityInputId::Cancel));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
	}

}

void AWobGasCharacter::BindAttributeListeners() const
{
	AbilitySystemComponent->OnZeroHealth.AddDynamic(this, &AWobGasCharacter::OnDeath);
}

// Called when the game starts or when spawned
void AWobGasCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	BindAttributeListeners();
}

// Called every frame
void AWobGasCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AWobGasCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	if (AbilitySystemComponent && InputComponent)
	{
		const FGameplayAbilityInputBinds Binds("Confirm", "Cancel", "EGasAbilityInputId", static_cast<int32>(EGasAbilityInputId::Confirm), static_cast<int32>(EGasAbilityInputId::Cancel));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
	}
}

