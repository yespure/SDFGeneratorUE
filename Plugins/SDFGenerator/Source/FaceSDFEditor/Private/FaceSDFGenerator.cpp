#include "FaceSDFGenerator.h"

#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"


static void ClearBottomLeftEarUV(
    TArray<uint8>& Pixels,
    int32 Resolution)
{
    if (Resolution <= 0 ||
        Pixels.Num() != Resolution * Resolution)
    {
        return;
    }

    /*
     * Unreal纹理坐标通常为：
     *
     * U = 0：最左边
     * U = 1：最右边
     * V = 0：最上边
     * V = 1：最下边
     *
     * 所以左下角是：
     * U较小，V较大。
     */
    const float EarMinU = 0.0f;
    const float EarMaxU = 0.20f;
    const float EarMinV = 0.75f;
    const float EarMaxV = 1.0f;

    for (int32 Y = 0; Y < Resolution; ++Y)
    {
        for (int32 X = 0; X < Resolution; ++X)
        {
            const float U = (static_cast<float>(X) + 0.5f) / static_cast<float>(Resolution);
            const float V = (static_cast<float>(Y) + 0.5f) / static_cast<float>(Resolution);
            const bool bIsEarRegion =
                U >= EarMinU &&
                U <= EarMaxU &&
                V >= EarMinV &&
                V <= EarMaxV;

            if (bIsEarRegion)
            {
                const int32 PixelIndex =
                    Y * Resolution + X;

                
                Pixels[PixelIndex] = 0;
            }
        }
    }
}

 bool FFaceSDFGenerator::RayIntersectsTriangle(
    const FVector3f& RayOrigin,
    const FVector3f& RayDirection,
    const FFaceSDFTriangle& Triangle)
{
    const FVector3f Edge1 = Triangle.Position1 - Triangle.Position0;
    const FVector3f Edge2 = Triangle.Position2 - Triangle.Position0;
    const FVector3f P = FVector3f::CrossProduct(RayDirection, Edge2);

    const float Det = FVector3f::DotProduct(Edge1, P);

    if (FMath::Abs(Det) < 0.000001f)
    {
        return false;
    }

    const float InvDet = 1.0f / Det;
    const FVector3f T = RayOrigin - Triangle.Position0;
    const float U = FVector3f::DotProduct(T, P) * InvDet;

    if (U < 0.0f || U > 1.0f)
    {
        return false;
    }

    const FVector3f Q = FVector3f::CrossProduct(T, Edge1);
    const float V = FVector3f::DotProduct(RayDirection, Q) * InvDet;

    if (V < 0.0f || U + V > 1.0f)
    {
        return false;
    }

    const float Distance =
        FVector3f::DotProduct(Edge2, Q) * InvDet;

    return Distance > 0.0001f;
}


bool FFaceSDFGenerator::ExtractFaceTriangles(
    USkeletalMesh* Mesh,
    int32 LODIndex,
    int32 SectionIndex,
    int32 UVChannel,
    TArray<FFaceSDFTriangle>& OutTriangles)
{
    if (!Mesh)//获取当前选中的模型
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: No Skeletal Mesh selected."));
        return false;
    }

    FSkeletalMeshModel* ImportedModel = Mesh->GetImportedModel();//得到模型信息和数据

    if (!ImportedModel)//Imported保护
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to get imported model."));
        return false;
    }

    if (LODIndex < 0 || LODIndex >= ImportedModel->LODModels.Num())//LOD验证保护
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid LOD index."));
        return false;
    }

    FSkeletalMeshLODModel& LODModel =
        ImportedModel->LODModels[LODIndex];//获取指定LOD的模型数据

    if (!LODModel.Sections.IsValidIndex(SectionIndex))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid Section index."));
        return false;
    }

    const FSkelMeshSection& Section =
        LODModel.Sections[SectionIndex];//获取指定Section的模型数据

    const int32 NumTriangles =
        static_cast<int32>(Section.NumTriangles);//获取三角面数量

    if (NumTriangles <= 0)//三角面数量保护
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Section contains no triangles."));
        return false;
    }

    if (UVChannel < 0 || UVChannel >= static_cast<int32>(LODModel.NumTexCoords))//UV保护
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid UV Channel."));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Face SDF: Mesh=%s LOD=%d Section=%d"),
        *Mesh->GetName(),
        LODIndex,
        SectionIndex);

    UE_LOG(LogTemp, Warning,
        TEXT("Face SDF: NumVertices=%d NumTriangles=%d"),
        Section.NumVertices,
        NumTriangles);

    OutTriangles.Reset();
    OutTriangles.Reserve(NumTriangles);//预分配三角形数组大小

    for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)//遍历所有三角形并加入数组
    {
        const uint32 IndexOffset =
            Section.BaseIndex + TriangleIndex * 3;//从整个LOD的IndexBuffer找到当前Section的三角形的索引偏移量

        if (IndexOffset + 2 >= static_cast<uint32>(LODModel.IndexBuffer.Num()))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Face SDF: IndexBuffer access out of range."));
            return false;
        }

        const uint32 Index0 = LODModel.IndexBuffer[IndexOffset];
        const uint32 Index1 = LODModel.IndexBuffer[IndexOffset + 1];
        const uint32 Index2 = LODModel.IndexBuffer[IndexOffset + 2];//获取三角形的三个顶点索引

        if (Index0 < Section.BaseVertexIndex ||
            Index1 < Section.BaseVertexIndex ||
            Index2 < Section.BaseVertexIndex)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Face SDF: Vertex index out of range."));
            return false;
        }

        const uint32 LocalVertex0 =
            Index0 - Section.BaseVertexIndex;

        const uint32 LocalVertex1 =
            Index1 - Section.BaseVertexIndex;

        const uint32 LocalVertex2 =
            Index2 - Section.BaseVertexIndex;//将全局顶点索引转换为Section内的局部顶点索引

        if (LocalVertex0 >= static_cast<uint32>(Section.SoftVertices.Num()) ||
            LocalVertex1 >= static_cast<uint32>(Section.SoftVertices.Num()) ||
            LocalVertex2 >= static_cast<uint32>(Section.SoftVertices.Num()))//检查
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Face SDF: SoftVertices access out of range."));
            return false;
        }

        const FSoftSkinVertex& Vertex0 =
            Section.SoftVertices[LocalVertex0];

        const FSoftSkinVertex& Vertex1 =
            Section.SoftVertices[LocalVertex1];

        const FSoftSkinVertex& Vertex2 =
            Section.SoftVertices[LocalVertex2];//获取三角形的三个顶点数据

        FFaceSDFTriangle Triangle;

        Triangle.UV0 = FVector2D(
            Vertex0.UVs[UVChannel].X,
            Vertex0.UVs[UVChannel].Y);

        Triangle.UV1 = FVector2D(
            Vertex1.UVs[UVChannel].X,
            Vertex1.UVs[UVChannel].Y);

        Triangle.UV2 = FVector2D(
            Vertex2.UVs[UVChannel].X,
            Vertex2.UVs[UVChannel].Y);//获取三角形的三个顶点的UV坐标

        // 获取三角形三个顶点的位置
        Triangle.Position0 = Vertex0.Position;
        Triangle.Position1 = Vertex1.Position;
        Triangle.Position2 = Vertex2.Position;

        // 获取三角形三个顶点的法线
        Triangle.Normal0 = Vertex0.TangentZ;
        Triangle.Normal1 = Vertex1.TangentZ;
        Triangle.Normal2 = Vertex2.TangentZ;

        OutTriangles.Add(Triangle);
    }

    return true;
}


//模型光栅化
bool FFaceSDFGenerator::RasterizeShadowMask(
    const TArray<FFaceSDFTriangle>& Triangles,
    const FVector3f& LightDirection,
    int32 Resolution,
    TArray<uint8>& OutPixels)
{
    if (Triangles.Num() == 0)//三角形数量检查
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: No triangles to rasterize."));
        return false;
    }

    if (Resolution <= 0)//分辨率检查
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid resolution."));
        return false;
    }

    const int32 Width = Resolution;
    const int32 Height = Resolution; //给生成的图片赋值

    OutPixels.Init(0, Width * Height);//初始化像素

    for (const FFaceSDFTriangle& Triangle : Triangles)//遍历所有三角形
    {
        const float MinU = FMath::Min3(
            Triangle.UV0.X,
            Triangle.UV1.X,
            Triangle.UV2.X);

        const float MaxU = FMath::Max3(
            Triangle.UV0.X,
            Triangle.UV1.X,
            Triangle.UV2.X);

        const float MinV = FMath::Min3(
            Triangle.UV0.Y,
            Triangle.UV1.Y,
            Triangle.UV2.Y);

        const float MaxV = FMath::Max3(
            Triangle.UV0.Y,
            Triangle.UV1.Y,
            Triangle.UV2.Y);//计算三角形的UV包围盒

        const int32 MinX = FMath::Clamp(
            FMath::FloorToInt(MinU * Width),
            0,
            Width - 1);

        const int32 MaxX = FMath::Clamp(
            FMath::FloorToInt(MaxU * Width),
            0,
            Width - 1);

        const int32 MinY = FMath::Clamp(
            FMath::FloorToInt(MinV * Height),
            0,
            Height - 1);

        const int32 MaxY = FMath::Clamp(
            FMath::FloorToInt(MaxV * Height),
            0,
            Height - 1);//计算包围盒在像素空间的范围

        const FVector2D A = Triangle.UV0;
        const FVector2D B = Triangle.UV1;
        const FVector2D C = Triangle.UV2;//获取三角形的三个顶点UV坐标

        const FVector2D AB = B - A;
        const FVector2D AC = C - A;//计算三角形的边向量

        const float Cross =
            AB.X * AC.Y - AB.Y * AC.X;//计算叉积用于判断三角形的面积和方向

        if (FMath::IsNearlyZero(Cross))//检查三角形是否退化
        {
            continue;
        }

        for (int32 Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32 X = MinX; X <= MaxX; ++X)
            {
                const FVector2D P(
                    (static_cast<float>(X) + 0.5f) / Width,
                    (static_cast<float>(Y) + 0.5f) / Height);//计算像素中心点的UV坐标

                const FVector2D AP = P - A;

                const float CrossAB =
                    AB.X * AP.Y - AB.Y * AP.X;//计算点P与边AB的叉积

                const FVector2D BP = P - B;
                const FVector2D BC = C - B;

                const float CrossBC =
                    BC.X * BP.Y - BC.Y * BP.X;//同理计算点P与边BC的叉积

                const FVector2D CP = P - C;
                const FVector2D CA = A - C;

                const float CrossCA =
                    CA.X * CP.Y - CA.Y * CP.X;//同理计算点P与边CA的叉积

                const bool bSameSign =
                    (CrossAB >= 0.0f &&
                        CrossBC >= 0.0f &&
                        CrossCA >= 0.0f)
                    ||
                    (CrossAB <= 0.0f &&
                        CrossBC <= 0.0f &&
                        CrossCA <= 0.0f);//检查点P是否在三角形内，若三个叉积符号相同则在三角形内

                if (bSameSign)
                {
                    const int32 PixelIndex =
                        Y * Width + X;

                    // 计算当前像素在三角形中的权重
                    const float W0 = CrossBC / Cross;
                    const float W1 = CrossCA / Cross;
                    const float W2 = CrossAB / Cross;

                    // 获取当前像素对应的面部法线
                    const FVector3f SurfaceNormal =
                        (
                            Triangle.Normal0 * W0 +
                            Triangle.Normal1 * W1 +
                            Triangle.Normal2 * W2
                            ).GetSafeNormal();

                    // 计算当前像素是否朝向光源
                    const FVector3f L =
                        LightDirection.GetSafeNormal();

                    const float NdotL =
                        FVector3f::DotProduct(SurfaceNormal, L);

                    const float LightThreshold = 0.25f;

                    bool bIsLit =
                        NdotL > LightThreshold;

                    if (bIsLit)
                    {
                        const FVector3f SurfacePosition =
                            Triangle.Position0 * W0 +
                            Triangle.Position1 * W1 +
                            Triangle.Position2 * W2;

                        const FVector3f RayOrigin = SurfacePosition + SurfaceNormal * 0.01f;

                        for (const FFaceSDFTriangle& OtherTriangle : Triangles)
                        {
                            if (&OtherTriangle == &Triangle)
                            {
                                continue;
                            }

                            if (RayIntersectsTriangle(
                                RayOrigin,
                                L,
                                OtherTriangle))
                            {
                                bIsLit = false;
                                break;
                            }
                        }
                    }

                    OutPixels[PixelIndex] = bIsLit ? 255 : 0;
                }
            }
        }
    }

    ClearBottomLeftEarUV(
        OutPixels,
        Resolution);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Face SDF: Rasterization finished. "
            "Resolution=%d, Triangles=%d"),
        Resolution,
        Triangles.Num());

    return true;
}


bool FFaceSDFGenerator::GenerateGrayscaleSDF(
    const TArray<uint8>& MaskPixels,
    int32 Resolution,
    TArray<uint8>& OutPixels)
{
    if (MaskPixels.Num() != Resolution * Resolution)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid mask size."));
        return false;
    }//检查Mask像素数量是否正确

    const int32 Width = Resolution;
    const int32 Height = Resolution;
    const int32 PixelCount = Width * Height;

    TArray<float> DistanceInside;
    TArray<float> DistanceOutside;//设定基本variables

    DistanceInside.Init(TNumericLimits<float>::Max(), PixelCount);
    DistanceOutside.Init(TNumericLimits<float>::Max(), PixelCount);//初始化距离数组

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const int32 Index = Y * Width + X;

            const bool bInside = MaskPixels[Index] > 127;

            if (bInside)
            {
                DistanceInside[Index] = 0.0f;
            }
            else
            {
                DistanceOutside[Index] = 0.0f;
            }
        }
    }//初始化距离数组，Mask内的点距离为0，外的点距离为最大值

    const float DiagonalCost = 1.41421356237f;//斜边检测距离. 8-connected

    for (int32 Y = 0; Y < Height; ++Y)//扫描左上
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const int32 Index = Y * Width + X;

            if (X > 0)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index - 1] + 1.0f);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index - 1] + 1.0f);
            }

            if (Y > 0)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index - Width] + 1.0f);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index - Width] + 1.0f);
            }

            if (X > 0 && Y > 0)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index - Width - 1] + DiagonalCost);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index - Width - 1] + DiagonalCost);
            }

            if (X + 1 < Width && Y > 0)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index - Width + 1] + DiagonalCost);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index - Width + 1] + DiagonalCost);
            }
        }
    }

    for (int32 Y = Height - 1; Y >= 0; --Y)//扫描右下
    {
        for (int32 X = Width - 1; X >= 0; --X)
        {
            const int32 Index = Y * Width + X;

            if (X + 1 < Width)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index + 1] + 1.0f);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index + 1] + 1.0f);
            }

            if (Y + 1 < Height)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index + Width] + 1.0f);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index + Width] + 1.0f);
            }

            if (X + 1 < Width && Y + 1 < Height)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index + Width + 1] + DiagonalCost);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index + Width + 1] + DiagonalCost);
            }

            if (X > 0 && Y + 1 < Height)
            {
                DistanceInside[Index] = FMath::Min(
                    DistanceInside[Index],
                    DistanceInside[Index + Width - 1] + DiagonalCost);

                DistanceOutside[Index] = FMath::Min(
                    DistanceOutside[Index],
                    DistanceOutside[Index + Width - 1] + DiagonalCost);
            }
        }
    }

    OutPixels.SetNumUninitialized(PixelCount);

    const float MaxDistance = 64.0f;

    for (int32 Index = 0; Index < PixelCount; ++Index)//正负号分配,确定面部边界
    {
        const bool bInside = MaskPixels[Index] > 127;

        float SignedDistance;

        if (bInside)
        {
            SignedDistance = DistanceOutside[Index];
        }
        else
        {
            SignedDistance = -DistanceInside[Index];
        }

        const float NormalizedDistance =
            FMath::Clamp(
                SignedDistance / MaxDistance,
                -1.0f,
                1.0f);

        const float Value =
            128.0f + NormalizedDistance * 127.0f;

        OutPixels[Index] =
            static_cast<uint8>(
                FMath::Clamp(
                    FMath::RoundToInt(Value),
                    0,
                    255));//生成
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Face SDF: Grayscale SDF generated. Resolution=%d"),
        Resolution);

    return true;
}