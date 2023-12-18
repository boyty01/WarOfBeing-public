// Fill out your copyright notice in the Description page of Project Settings.


#include "Execution/GEC_GrantExperience.h"
#include "AttributeSet/WobGasAttributes.h"

struct FExperienceAwardStruct
{
    //The DECLARE_ATTRIBUTE_CAPTUREDEF macro actually only declares two variables. The variable names are dependent on the input, however. Here they will be HealthProperty(which is a UPROPERTY pointer)
    //and HealthDef(which is a FGameplayEffectAttributeCaptureDefinition).
    DECLARE_ATTRIBUTE_CAPTUREDEF(ExperiencePoints);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CharacterLevel);
    DECLARE_ATTRIBUTE_CAPTUREDEF(BaseExperienceWorth);

    // Macros dont account for retrieving source and target attributes of the same type, so we have to define these manually.
    FProperty* SourceCharacterLevelProperty;
    FGameplayEffectAttributeCaptureDefinition SourceCharacterLevelDef;

    FExperienceAwardStruct()
    {
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWobGasAttributes, ExperiencePoints, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWobGasAttributes, CharacterLevel, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UWobGasAttributes, BaseExperienceWorth, Source, false);

        // Manually grabbing the attribute data for the source character level. 
        SourceCharacterLevelProperty = FindFieldChecked<FProperty>(UWobGasAttributes::StaticClass(), GET_MEMBER_NAME_CHECKED(UWobGasAttributes, CharacterLevel));
        SourceCharacterLevelDef = FGameplayEffectAttributeCaptureDefinition(SourceCharacterLevelProperty, EGameplayEffectAttributeCaptureSource::Source, false);
    }
};

void UGEC_GrantExperience::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    float CharacterLevelRec;
    float CharacterLevelSrc;
    float ExperiencePoints;
    float BaseExperienceAward;

    FAggregatorEvaluateParameters EvaluationParameters; //We use these tags to set up an FAggregatorEvaluateParameters struct, which we will need to get the values of our captured attributes later in this function.

    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags(); //Some more helper variables: Spec is the spec this execution originated from, and the Source/TargetTags are pointers to the tags granted to source/target actor, respectively.

    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(FExperienceAwardStruct().CharacterLevelDef, EvaluationParameters, CharacterLevelRec);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(FExperienceAwardStruct().SourceCharacterLevelDef, EvaluationParameters, CharacterLevelSrc);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(FExperienceAwardStruct().ExperiencePointsDef, EvaluationParameters, ExperiencePoints);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(FExperienceAwardStruct().BaseExperienceWorthDef, EvaluationParameters, BaseExperienceAward);

    // Edit this formula to affect the global experience award based on the level difference.
    float FinalExperience = FMath::Clamp(BaseExperienceAward * (CharacterLevelSrc - (CharacterLevelRec * 0.1f)), 0, FLT_MAX); 

    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(FExperienceAwardStruct().ExperiencePointsProperty, EGameplayModOp::Additive, -FinalExperience));
}
