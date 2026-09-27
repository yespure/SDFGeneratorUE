#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"
//用于产生SDF的65个光照方向
//生成ShadowMask的和Atlas的必须使用同一份采样结果
class FFaceSDFLightSampler
{
public:
	static FVector3f MakeDirection(
		float RelativeYawDegrees,
		float PitchDegrees
	);
	static void BuildDefaultSamples(
		TArray<FFaceSDFLightSample>& OutSamples);
};