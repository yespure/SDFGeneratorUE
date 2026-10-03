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

int32 FFaceSDFGenerator::BuildBVHRecursive(
    const TArray<FFaceSDFTriangle>& Triangles,
    const TArray<int32>& TriangleIndices,
    TArray<FFaceSDFBVHNode>& OutNodes)
{
    if (TriangleIndices.IsEmpty())
    {
        return INDEX_NONE;
    }

    for (int32 TriangleIndex : TriangleIndices)
    {
        if (!Triangles.IsValidIndex(TriangleIndex))
        {
            return INDEX_NONE;
        }
    }

    const int32 NodeIndex = OutNodes.AddDefaulted();
    FFaceSDFBVHNode& Node = OutNodes[NodeIndex];

    FVector3f BoundsMin(
        TNumericLimits<float>::Max(),
        TNumericLimits<float>::Max(),
        TNumericLimits<float>::Max());

    FVector3f BoundsMax(
        -TNumericLimits<float>::Max(),
        -TNumericLimits<float>::Max(),
        -TNumericLimits<float>::Max());

    for (int32 TriangleIndex : TriangleIndices)
    {
        const FFaceSDFTriangle& Triangle = Triangles[TriangleIndex];
        const FVector3f Positions[3] =
        {
            Triangle.Position0,
            Triangle.Position1,
            Triangle.Position2
        };//获取三角形顶点位置

        for (const FVector3f& Position : Positions)
        {
            BoundsMin.X = FMath::Min(BoundsMin.X, Position.X);
            BoundsMin.Y = FMath::Min(BoundsMin.Y, Position.Y);
            BoundsMin.Z = FMath::Min(BoundsMin.Z, Position.Z);

            BoundsMax.X = FMath::Max(BoundsMax.X, Position.X);
            BoundsMax.Y = FMath::Max(BoundsMax.Y, Position.Y);
            BoundsMax.Z = FMath::Max(BoundsMax.Z, Position.Z);
        }//通过遍历三角形的三个顶点，更新包围盒的最小和最大坐标,框住
    }

    Node.BoundsMin = BoundsMin;
    Node.BoundsMax = BoundsMax;
    //Leaf
    if (TriangleIndices.Num() <= 4)
    {
        Node.TriangleIndices = TriangleIndices;
        return NodeIndex;
    }//控制树深度,每个叶子节点最多包含4个三角形

    const FVector3f Extent = BoundsMax - BoundsMin;

    int32 SplitAxis = 0;

    if (Extent.Y > Extent.X)
    {
        SplitAxis = 1;
    }

    if (Extent.Z > Extent[SplitAxis])
    {
        SplitAxis = 2;
    }//split类型分类, 根据包围盒的最大边长选择分割轴

    TArray<int32> SortedIndices = TriangleIndices;

    SortedIndices.Sort(
        [&Triangles, SplitAxis](int32 A, int32 B)
        {
            const FFaceSDFTriangle& TA = Triangles[A];
            const FFaceSDFTriangle& TB = Triangles[B];

            const FVector3f CenterA = (TA.Position0 + TA.Position1 + TA.Position2) / 3.0f;
            const FVector3f CenterB = (TB.Position0 + TB.Position1 + TB.Position2) / 3.0f;
            return CenterA[SplitAxis] < CenterB[SplitAxis];
        });//根据三角形的中心点在分割轴上的位置对三角形索引进行排序

    const int32 Middle = SortedIndices.Num() / 2;//将排序后的三角形索引数组分为两半
    TArray<int32> LeftIndices;
    TArray<int32> RightIndices;

    LeftIndices.Append(SortedIndices.GetData(), Middle);
    RightIndices.Append(SortedIndices.GetData() + Middle, SortedIndices.Num() - Middle);//将前半部分放入LeftIndices，后半部分放入RightIndices

    const int32 LeftChildIndex = BuildBVHRecursive(
        Triangles,
        LeftIndices,
        OutNodes);
    const int32 RightChildIndex = BuildBVHRecursive(
        Triangles,
        RightIndices,//递归构建左右子树
        OutNodes);
    OutNodes[NodeIndex].LeftChild = LeftChildIndex;
    OutNodes[NodeIndex].RightChild = RightChildIndex;//设置当前节点的左右子节点索引
    return NodeIndex;//返回当前节点的索引
}

bool FFaceSDFGenerator::RayIntersectAABB(
    const FVector3f& RayOrigin,
    const FVector3f& RayDirection,
    const FVector3f& BoundsMin,
    const FVector3f& BoundsMax
)
{
    float TMin = 0.0f;
    float TMax = TNumericLimits<float>::Max();//初始化射线的有效距离范围

    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        if (RayDirection[Axis] == 0.0f)
        {
            if (RayOrigin[Axis] < BoundsMin[Axis] ||
                RayOrigin[Axis] > BoundsMax[Axis])
            {
                return false;
            }

            continue;
        }//如果射线在当前轴上平行于AABB的面，则检查射线原点是否在AABB的范围内，如果不在则返回false

        const float InvDirection = 1.0f / RayDirection[Axis];
        float T0 = (BoundsMin[Axis] - RayOrigin[Axis]) *
            InvDirection;//计算射线与AABB在当前轴上的交点t0和t1

        float T1 = (BoundsMax[Axis] - RayOrigin[Axis]) * InvDirection;

        if (T0 > T1)
        {
            Swap(T0, T1);
        }//确保T0是较小的值，T1是较大的值

        TMin = FMath::Max(TMin, T0);
        TMax = FMath::Min(TMax, T1);

        if (TMax < TMin)
        {
            return false;
        }//更新TMin和TMax，确保它们在所有轴上都有效，如果在某个轴上TMax小于TMin，则射线与AABB不相交，返回false

    }

    return true;
}

bool FFaceSDFGenerator::TraceBVH(
    const FVector3f& RayOrigin,
    const FVector3f& RayDirection,
    const TArray<FFaceSDFTriangle>& Triangles,
    const TArray<FFaceSDFBVHNode>& Nodes,
    int32 RootNodeIndex,
    int32 IgnoreTriangleIndex)
{
    if (!Nodes.IsValidIndex(RootNodeIndex))
    {
        return false;
    }

    TArray<int32> Stack;
    Stack.Reserve(64);
    Stack.Add(RootNodeIndex);//初始化栈并将根节点索引压入栈中

    while (Stack.Num() > 0)
    {
        const int32 NodeIndex = Stack.Last();
        Stack.RemoveAt(Stack.Num() - 1);
        const FFaceSDFBVHNode& Node = Nodes[NodeIndex];

        if (!RayIntersectAABB(
            RayOrigin,
            RayDirection,
            Node.BoundsMin,
            Node.BoundsMax
        ))
        {
            continue;
        }

        if (Node.IsLeaf())
        {
            for (int32 TriangleIndex : Node.TriangleIndices)
            {
                if (TriangleIndex == IgnoreTriangleIndex)
                {
                    continue;
                }

                if (RayIntersectsTriangle(
                    RayOrigin,
                    RayDirection,
                    Triangles[TriangleIndex]))
                {
                    return true;
                }
            }
        }
        else
        {
            if (Node.LeftChild != INDEX_NONE)
            {
                Stack.Add(Node.LeftChild);
            }

            if (Node.RightChild != INDEX_NONE)
            {
                Stack.Add(Node.RightChild);
            }
        }
    }

    return false;

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
    const TArray<FFaceSDFBVHNode>& BVHNodes,
    int32 RootNodeIndex,
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

    if (Resolution <= 0 || static_cast<int64>(Resolution) * Resolution > MAX_int32)//分辨率检查
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid resolution."));
        return false;
    }

    if (!BVHNodes.IsValidIndex(RootNodeIndex))//BVH检查
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid BVH root node."));
        return false;
    }

    const int32 Width = Resolution;
    const int32 Height = Resolution; //给生成的图片赋值

    OutPixels.Init(0, Width * Height);//初始化像素

    // 一个ShadowMask只使用一个LightDirection，所以只需要Normalize一次
    const FVector3f L =
        LightDirection.GetSafeNormal();

    for (int32 TriangleIndex = 0;
        TriangleIndex < Triangles.Num();
        ++TriangleIndex)//遍历所有三角形
    {
        const FFaceSDFTriangle& Triangle =
            Triangles[TriangleIndex];

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
                    const float NdotL =
                        FVector3f::DotProduct(
                            SurfaceNormal,
                            L);

                    const float LightThreshold = 0.25f;

                    bool bIsLit =
                        NdotL > LightThreshold;

                    if (bIsLit)
                    {
                        const FVector3f SurfacePosition =
                            Triangle.Position0 * W0 +
                            Triangle.Position1 * W1 +
                            Triangle.Position2 * W2;

                        const FVector3f RayOrigin =
                            SurfacePosition +
                            SurfaceNormal * 0.01f;

                        // 使用BVH寻找可能与Ray相交的三角形
                        // 最终仍然使用RayIntersectsTriangle进行精确求交
                        if (TraceBVH(
                            RayOrigin,
                            L,
                            Triangles,
                            BVHNodes,
                            RootNodeIndex,
                            TriangleIndex))
                        {
                            bIsLit = false;
                        }
                    }

                    OutPixels[PixelIndex] =
                        bIsLit ? 255 : 0;
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
            "Resolution=%d, Triangles=%d, BVHNodes=%d"),
        Resolution,
        Triangles.Num(),
        BVHNodes.Num());

    return true;
}


bool FFaceSDFGenerator::GenerateGrayscaleSDF(
    const TArray<uint8>& MaskPixels,
    int32 Resolution,
    TArray<uint8>& OutPixels)
{
    if (Resolution <= 0 ||
        static_cast<int64>(Resolution) * Resolution > MAX_int32 ||
        MaskPixels.Num() != static_cast<int64>(Resolution) * Resolution)
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

