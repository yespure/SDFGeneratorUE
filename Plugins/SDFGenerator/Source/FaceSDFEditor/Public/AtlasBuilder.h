#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"
//灰度图进入指定Atlas格子
class FFaceSDFAtlasBuilder
{
public:
    static bool Build(
        const TArray<FFaceSDFGrayImage>& Images,
        const TArray<FFaceSDFLightSample>& Samples,
        int32 GridSize,
        FFaceSDFGrayImage& OutAtlas,
        FString& OutError);

private:
    static bool CopyTile(
        const FFaceSDFGrayImage& Source,
        int32 TargetRow,
        int32 TargetColumn,
        int32 GridSize,
        FFaceSDFGrayImage& Atlas,
        FString& OutError);
};