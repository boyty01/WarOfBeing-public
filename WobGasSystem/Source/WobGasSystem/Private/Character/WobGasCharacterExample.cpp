// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/WobGasCharacterExample.h"
#include "PlayerState/WobGasPlayerState.h"

// Sets default values
AWobGasCharacterExample::AWobGasCharacterExample()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

UAbilitySystemComponent* AWobGasCharacterExample::GetAbilitySystemComponent() const
{
	if (GetPlayerState() && Cast<AWobGasPlayerState>(GetPlayerState()))
		return Cast<AWobGasPlayerState>(GetPlayerState())->GetAbilitySystemComponent();
	
	return nullptr;
}

// Called when the game starts or when spawned
void AWobGasCharacterExample::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* AWobGasCharacterExample::K2_GetAbilitySystemComponent_Implementation() const
{
	return GetAbilitySystemComponent();
}

// Called every frame
void AWobGasCharacterExample::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AWobGasCharacterExample::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

