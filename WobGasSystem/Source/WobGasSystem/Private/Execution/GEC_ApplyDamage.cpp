// Fill out your copyright notice in the Description page of Project Settings.


#include "Execution/GEC_ApplyDamage.h"
#include "AttributeSet/WobGasAttributes.h"

struct FDamageAttributeStruct
{
    //The DECLARE_ATTRIBUTE_CAPTUREDEF macro actually only declares two variables. The variable names are dependent on the input, however. Here they will be HealthProperty(which is a UPROPERTY pointer)
    //and HealthDef(which is a FGameplayEffectAttributeCaptureDefinition).
    DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
    DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalResistance); 

    FDamageAttributeStruct()
    {
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWobGasAttributes, Health, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWobGasAttributes, PhysicalResistance, Target, false);

    }
};

UGEC_ApplyDamage::UGEC_ApplyDamage()
{

    FDamageAttributeStruct Attributes;

    RelevantAttributesToCapture.Add(Attributes.HealthDef);
    RelevantAttributesToCapture.Add(Attributes.PhysicalResistanceDef);
}

void UGEC_ApplyDamage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    float PhysicalResistance;
    FAggregatorEvaluateParameters EvaluationParameters; //We use these tags to set up an FAggregatorEvaluateParameters struct, which we will need to get the values of our captured attributes later in this function.

    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags(); //Some more helper variables: Spec is the spec this execution originated from, and the Source/TargetTags are pointers to the tags granted to source/target actor, respectively.

    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(FDamageAttributeStruct().PhysicalResistanceDef, EvaluationParameters, PhysicalResistance);

    float BaseDamage = Spec.GetModifierMagnitude(0, false); // this is expected to be the effect mag specifier attribute. 
    float Mitigation = BaseDamage * PhysicalResistance; 
    float FinalDamage = BaseDamage - (Mitigation); 
    
    UE_LOG(LogTemp, Warning, TEXT("resistance %f, mitigation %f"), PhysicalResistance, Mitigation);
    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(FDamageAttributeStruct().HealthProperty, EGameplayModOp::Additive, -FinalDamage));

}
