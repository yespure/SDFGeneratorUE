#pragma once

#include "CoreMinimal.h"

//Skeletal Mesh提取三角面, position normal负责计算,位于模型局部空间;
//UV位于UV通道
struct FFaceSDFTriangle
{
    FVector2D UV0;
    FVector2D UV1;
    FVector2D UV2;

    // 三角形三个顶点的模型局部空间位置
    FVector3f Position0;
    FVector3f Position1;
    FVector3f Position2;

    // 三角形三个顶点的模型局部空间法线
    FVector3f Normal0;
    FVector3f Normal1;
    FVector3f Normal2;
};//三角形信息

struct FFaceSDFGrayImage
{
    int32 Width = 0;
    int32 Height = 0;

    TArray<uint8> Pixels;

    bool IsValid() const
    {
        return Width > 0
            && Height > 0
            && Pixels.Num() == Width * Height;
    }

    bool IsSquare() const
    {
        return IsValid() && Width == Height;
    }

    void Initialize(
        int32 InWidth,
        int32 InHeight,
        uint8 InitialValue = 0
    )
    {
        Width = InWidth;
        Height = InHeight;
        //Grayscale图像初始化
        Pixels.Init(
            InitialValue,
            Width * Height
        );
    }
};
//光照采样方向和他在Atlas中的位置
struct FFaceSDFLightSample
{
    FVector3f Direction = FVector3f::ZeroVector;
    int32 AtlasRow = 0;
    int32 AtlasColumn = 0;

    FString OutputName;
}
//模型数据读取
struct FFaceSDFMeshSettings
{
    int32 LODIndex = 0;
    int32 SectionIndex = 0;
    int32 UVChannel = 0;
};

struct FFaceSDFGenerationSettings
{
    int32 Resolution = 128;
    int32 AtlasGridSize = 9;

    bool bClearBottomLeftRegion = true;

    float ClearMinU = 0.0f;
    float ClearMaxU = 0.2f;
    float ClearMinV = 0.75f;
    float ClearMaxV = 1.0f;
};

struct FFaceSDFOperationResult
{
    bool bSucceeded = false;
    //生成结果检测
    int32 SucceededCount = 0;
    int32 FailedCount = 0;

    FString ErrorMessage;
    //如果成功 调用method
    static FFaceSDFOperationResult Success(int32 InSucceededCount = 0)
    {
        FFacedSDFOperationResult Result;
        Result.bSucceeded = true;
        Result.SucceededCount = InSucceededCount;

        return Result;
    }
    //如果失败
    static FFaceSDFOperationResult Failure(
        const FString& InErrorMessage)
    {
        FFaceSDFOperationResult Result;
        Result.bSucceeded = false;
        Result.FailedCount = 1;
        Result.ErrorMessage =
            InErrorMessage;

        return Result;
    }
};