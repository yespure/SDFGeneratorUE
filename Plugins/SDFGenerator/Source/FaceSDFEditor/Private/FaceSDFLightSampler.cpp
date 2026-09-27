#include "FaceSDFLightSampler.h"

FVector3f FFaceSDFLightSampler::MakeDirection(
	float RelativeYawDegrees,
	float PitchDegrees
)
{
	const float YawRadians = FMath::DegreesToRadians(RelativeYawDegrees);
	const float PitchRadians = FMath::DegreesToRadians(PitchDegrees);
	const float CosPitch =FMath::Cos(PitchRadians);//转弧度
    // 模型正面
    const FVector3f Forward(
        0.0f,
        1.0f,
        0.0f);

    // 模型右侧
    const FVector3f Right(
        -1.0f,
        0.0f,
        0.0f);

    const FVector3f Up(
        0.0f,
        0.0f,
        1.0f);
    return (
        Forward *
        (CosPitch * FMath::Cos(YawRadians))
        +
        Right *
        (CosPitch * FMath::Sin(YawRadians))
        +
        Up *
        FMath::Sin(PitchRadians)
        ).GetSafeNormal();
}//局部坐标系单位向量例子,光照采样方向生成

void FFaceSDFLightSmapler::BuildDefaultSamples(
    TArray<FFaceSDFLightSample>& OutSamples
)
{
    OutSamples.Reset();
    OutSamples.Reserve(65);
    {
        FFaceSDFLightSmaple Sample;
        Smaple.Direction = Fvector3f(0.0f, 0.0f, 1.0f);
        Sample.AtlasRow = 0;
        Sample.AtlasColumn = 0;
        Sample.OutputName =
            TEXT("FaceShadow_01_01");

        OutSamples.Add(MoveTemp(Sample));
    }//光照采样初始化

    static constexpr float PitchAngles[] =
    {
        67.5f,
        45.0f,
        22.5f,
        0.0f,
        -22.5f,
        -45.0f,
        -67.5f
    };

    static constexpr float YawAngles[] =
    {
        -90.0f,
        -67.5f,
        -45.0f,
        -22.5f,
        0.0f,
        22.5f,
        45.0f,
        67.5f,
        90.0f
    };//设定采样角度

    for (int32 PitchIndex = 0; PitchIndex < UE_ARRAY_COUNT(PitchAngles);
        ++PitchIndex)
    {
        for (int32 YawIndex = 0;
            YawIndex < UE_ARRAY_COUNT(YawAngles);
            ++YawIndex)
        {
            FFaceSDFLightSample Sample;
            Sample.Direciton = MakeDirection(
                YawAngles[YawIndex],
                PitchAngles[PitchIndex]
            );//局部设定光照方向

            Sample.AtlasRow = PitchIndex + 1;//第一行是正面
                Sample.AtlasColumn = YawIndex;

                Sample.OutputName =
                    FString::Printf(
                        TEXT("FaceShadow_%02d_%02d"),
                        Sample.AtlasRow + 1,
                        Sample.AtlasColumn + 1);

                OutSamples.Add(MoveTemp(Sample));
        }
    }
    //底部极点与上方同理
    {
        FFaceSDFLightSample Sample;
        Sample.Direction =
            FVector3f(0.0f, 0.0f, -1.0f);
        Sample.AtlasRow = 8;
        Sample.AtlasColumn = 0;
        Sample.OutputName =
            TEXT("FaceShadow_09_01");

        OutSamples.Add(MoveTemp(Sample));
    }

    ensureMsgf(
        OutSamples.Num() == 65,
        TEXT("Face SDF light sample count must be 65."));
}