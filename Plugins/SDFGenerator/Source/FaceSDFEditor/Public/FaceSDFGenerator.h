#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"

class USkeletalMesh;

struct FFaceSDFBVHNode
{
    FVector3f BoundsMin;
    FVector3f BoundsMax;

    int32 LeftChild = INDEX_NONE;
	int32 RightChild = INDEX_NONE;
    
    TArray<int32> TriangleIndices;

    bool IsLeaf() const
    {
        return LeftChild == INDEX_NONE && RightChild == INDEX_NONE;
	}
};

class FFaceSDFGenerator
{
public:
    //BVH分区构建
    static int32 BuildBVHRecursive(
        const TArray<FFaceSDFTriangle>& Triangles,
        const TArray<int32>& TruangleIndices,
        TArray<FFaceSDFBVHNode>& OutNodes
    );
	//光线与AABB相交检测
    static bool RayIntersectAABB(
        const FVector3f& RayOrigin,
        const FVector3f& RayDirection,
        const FVector3f& BoundsMin,
        const FVector3f& BoundsMax
    );
	//BVH光线追踪
    static bool TraceBVH(
        const FVector3f& RayOrigin,
        const FVector3f& RayDirection,
        const TArray<FFaceSDFTriangle>& Triangles,
        const TArray<FFaceSDFBVHNode>& Nodes,
        int32 RootNodeIndex,
        int32 IgnoreTriangleIndex
        );


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
        const TArray<FFaceSDFBVHNode>& BVHNodes,
        int32 RootNodeIndex,
        const FVector3f& LightDirection,
        int32 Resolution,
        TArray<uint8>& OutPixels);

    //灰度SDF
    static bool GenerateGrayscaleSDF(
        const TArray<uint8>& MaskPixels,
        int32 Resolution,
        TArray<uint8>& OutPixels);
};