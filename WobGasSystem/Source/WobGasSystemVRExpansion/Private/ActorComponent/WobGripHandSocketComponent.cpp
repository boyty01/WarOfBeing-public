// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/WobGripHandSocketComponent.h"

UWobGripHandSocketComponent::UWobGripHandSocketComponent(const FObjectInitializer& ObjectInitializer) : UHandSocketComponent(ObjectInitializer)
{
	/*
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> VisualizationMeshObj(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/MannequinsXR/Meshes/SKM_QuinnXR_right.SKM_QuinnXR_right'"));
	VisualizationMesh = VisualizationMeshObj.Object;
	HandRelativePlacement = FTransform(FRotator(20, 0, 80), FVector(-1.120374f, 2.826352f, 7.41304f), FVector(1));

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DefaultAnim(TEXT("/Script/Engine.AnimSequence'/Game/Characters/MannequinsXR/Meshes/SK_MannequinsXR_Sequence_base.SK_MannequinsXR_Sequence_base'"));
	HandTargetAnimation = DefaultAnim.Object;
	*/
}
