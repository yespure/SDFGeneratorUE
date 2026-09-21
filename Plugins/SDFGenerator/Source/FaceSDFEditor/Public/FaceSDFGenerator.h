#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"

class USkeletalMesh;

class FFaceSDFGenerator
{
public:

    static bool RayIntersectsTriangle(
        const FVector3f& RayOrigin,
        const FVector3f& RayDirection,
        const FFaceSDFTriangle& Triangle);
    


    //获取模型三角面数量
    static bool ExtractFaceTriangles(
        USkeletalMesh* Mesh,
        int32 LODIndex,
        int32 SectionIndex,
        int32 UVChannel,
        TArray<FFaceSDFTriangle>& OutTriangles);

    //模型光栅化
    static bool RasterizeShadowMask(
        const TArray<FFaceSDFTriangle>& Triangles,
        const FVector3f& LightDirection,
        int32 Resolution,
        TArray<uint8>& OutPixels);

    //灰度SDF
    static bool GenerateGrayscaleSDF(
        const TArray<uint8>& MaskPixels,
        int32 Resolution,
        TArray<uint8>& OutPixels);
};