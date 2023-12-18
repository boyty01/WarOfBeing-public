// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneComponent/HandComponent.h"
#include "Components/SphereComponent.h"
#include "Misc/VREPhysicsConstraintComponent.h"

// Sets default values for this component's properties
UHandComponent::UHandComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetAttachmentRoot());

	PhysicsRoot = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsRoot"));
	PhysicsRoot->SetupAttachment(Mesh);

	PhysicsConstraint = CreateDefaultSubobject<UVREPhysicsConstraintComponent>(TEXT("PhysicsConstraint"));
	PhysicsConstraint->SetupAttachment(Mesh);



	// ...
}


// Called when the game starts
void UHandComponent::BeginPlay()
{
	Super::BeginPlay();

	Mesh->OnComponentHit.AddDynamic(this, &UHandComponent::OnMeshHit);
	// ...
	
	

}


// Called every frame
void UHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

