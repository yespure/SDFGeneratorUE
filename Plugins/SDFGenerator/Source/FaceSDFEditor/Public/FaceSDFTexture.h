#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"

class FFaceSDFTexture
{
public:
    static bool SaveGrayscaleTexture(
        const FFaceSDFGrayImage& Image,
        const FString& PackageDirectory,
        const FString& AssetName,
        FString& OutError);
};