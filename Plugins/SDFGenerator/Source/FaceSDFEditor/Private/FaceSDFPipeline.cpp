#include "FaceSDFPipeline.h"

#include "AtlasBuilder.h"
#include "FaceSDFGenerator.h"
#include "FaceSDFLightSampler.h"

#include "Engine/SkeletalMesh.h"

namespace FaceSDFPipelinePrivate
{
    /**
         * 将指定UV矩形区域填充为固定灰度。
         *
         * 这用于清理当前模型UV左下角的耳朵。
         * 将来Painter也可以代替这个自动清理步骤。
         */
    static void FillUVRegion(
        FFaceSDFGrayImage& Image,
        float MinU,
        float MaxU,
        float MinV,
        float MaxV,
        uint8 FillValue)
    {
        if (!Image.IsValid())
        {
            return;
        }

        for (int32 Y = 0;
            Y < Image.Height;
            ++Y)
        {
            for (int32 X = 0;
                X < Image.Width;
                ++X)
            {
                const float U = (static_cast<float>(X) + 0.5f) / static_cast<float>(Image.Width);
                const float V = (static_cast<float>(Y) + 0.5f) / static_cast<float>(Image.Height);//划定清理范围
                const bool bInsideRegion = U >= MinU && U <= MaxU && V >= MinV && V <= MaxV;//判断是否在范围内
                if (bInsideRegion)//如果在清除
                {
                    const int32 PixelIndex =
                        Y * Image.Width + X;

                    Image.Pixels[PixelIndex] =
                        FillValue;
                }

            }
        }
    }

}

FFaceSDFOperationResult
FFaceSDFPipeline::GenerateShadowMasks(
    const FFaceSDFPipelineRequest& Request,
    TArray<FFaceSDFGrayImage>& OutMasks,
    TArray<FFaceSDFLightSample>& OutSamples)
{
    OutMasks.Reset();
    OutSamples.Reset();

    if (!Request.Mesh)
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("No Skeletal Mesh was provided."));
    }

    if (Request.GenerationSettings.Resolution <= 0)
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("Resolution must be greater than zero."));
    }

    TArray<FFaceSDFTriangle> FaceTriangles;

    if (!FFaceSDFGenerator::ExtractFaceTriangles(
        Request.Mesh,
        Request.MeshSettings.LODIndex,
        Request.MeshSettings.SectionIndex,
        Request.MeshSettings.UVChannel,
        FaceTriangles))
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("Failed to extract face triangles."));
    }//调用Generator函数提取面部三角形,并取出

    if (FaceTriangles.IsEmpty())
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("The selected mesh section contains no triangles."));
    }

    TArray<int32> TriangleIndices;
    TriangleIndices.Reserve(FaceTriangles.Num());

    for (int32 Index = 0; Index < FaceTriangles.Num(); ++Index)
    {
        TriangleIndices.Add(Index);
    }

    TArray<FFaceSDFBVHNode> BVHNodes;

    const int32 RootNodeIndex =
        FFaceSDFGenerator::BuildBVHRecursive(
            FaceTriangles,
            TriangleIndices,
            BVHNodes);

    if (!BVHNodes.IsValidIndex(RootNodeIndex))
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("Failed to build face BVH."));
    }

    FFaceSDFLightSampler::BuildDefaultSamples(
        OutSamples);//设定光照sample

    if (OutSamples.IsEmpty())
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("No light samples were generated."));
    }

    OutMasks.Reserve(OutSamples.Num());
    int32 GeneratedCount = 0;

    for (const FFaceSDFLightSample& Sample : OutSamples)//循环samples进行光栅化,为每个光照方向生成一个阴影遮罩
    {
        FFaceSDFGrayImage Mask;
        Mask.Width =
            Request.GenerationSettings.Resolution;
        Mask.Height =
            Request.GenerationSettings.Resolution;

        if (!FFaceSDFGenerator::RasterizeShadowMask(
            FaceTriangles,
            BVHNodes,
            RootNodeIndex,
            Sample.Direction,
            Request.GenerationSettings.Resolution,
            Mask.Pixels))
        {
            return FFaceSDFOperationResult::Failure(
                FString::Printf(
                    TEXT("Failed to generate Shadow Mask: %s"),
                    *Sample.OutputName));
        }//调用光栅化

        if (!Mask.IsValid())
        {
            return FFaceSDFOperationResult::Failure(
                FString::Printf(
                    TEXT("Generated Shadow Mask is invalid: %s"),
                    *Sample.OutputName));
        }

        if (Request.GenerationSettings.bClearBottomLeftRegion)
        {
            FaceSDFPipelinePrivate::FillUVRegion(
                Mask,
                Request.GenerationSettings.ClearMinU,
                Request.GenerationSettings.ClearMaxU,
                Request.GenerationSettings.ClearMinV,
                Request.GenerationSettings.ClearMaxV,
                0);
        }//如果需要清理左下耳朵,选择并进行清理

        OutMasks.Add(
            MoveTemp(Mask));//加入数组

        ++GeneratedCount;
    }

    return FFaceSDFOperationResult::Success(
        GeneratedCount);
}
FFaceSDFOperationResult
FFaceSDFPipeline::ConvertMasksToSDF(
    const TArray<FFaceSDFGrayImage>& Masks,
    TArray<FFaceSDFGrayImage>& OutSDFImages)
{
    OutSDFImages.Reset();
    if(Masks.IsEmpty())
    {
        return FFaceSDFOperationResult::Failure(
            TEXT("No Shadow Masks were provided."));
    }
    OutSDFImages.Reserve(Masks.Num());
    int32 ConvertedCount = 0;
    for (int32 ImageIndex = 0; ImageIndex < Masks.Num(); ++ImageIndex)
    {
        const FFaceSDFGrayImage& Mask =
            Masks[ImageIndex];

        if (!Mask.IsValid())
        {
            return FFaceSDFOperationResult::Failure(
                FString::Printf(
                    TEXT("Shadow Mask %d is invalid."),
                    ImageIndex));
        }

        if (!Mask.IsSquare())
        {
            return FFaceSDFOperationResult::Failure(
                FString::Printf(
                    TEXT(
                        "Shadow Mask %d is not square."),
                    ImageIndex));
        }//保护

        FFaceSDFGrayImage SDFImage;

        SDFImage.Width =
            Mask.Width;

        SDFImage.Height =
            Mask.Height;//创建一个SDFImage

        if (!FFaceSDFGenerator::GenerateGrayscaleSDF(
            Mask.Pixels,
            Mask.Width,
            SDFImage.Pixels))//调用灰度图算法
        {
            return FFaceSDFOperationResult::Failure(
                FString::Printf(
                    TEXT(
                        "Failed to convert Shadow Mask %d to SDF."),
                    ImageIndex));
        }

        if (!SDFImage.IsValid())
        {
            return FFaceSDFOperationResult::Failure(
                FString::Printf(
                    TEXT(
                        "Generated SDF image %d is invalid."),
                    ImageIndex));
        }
        OutSDFImages.Add(MoveTemp(SDFImage));//加入数组

        ++ConvertedCount;


    }

    return FFaceSDFOperationResult::Success(
        ConvertedCount);
}

FFaceSDFOperationResult
FFaceSDFPipeline::BuildAtlas(
    const TArray<FFaceSDFGrayImage>& SDFImages,
    const TArray<FFaceSDFLightSample>& Samples,
    FFaceSDFGrayImage& OutAtlas
)
{
    OutAtlas.Reset();

    FString Error;

    if (!FFaceSDFAtlasBuilder::Build(
        SDFImages,
        Samples,
        9,
        OutAtlas,
        Error))
    {
        return FFaceSDFOperationResult::Failure(
            Error);
    }

    if (!OutAtlas.IsValid())
    {
        return FFaceSDFOperationResult::Failure(
            TEXT(
                "Atlas Builder returned an invalid image."));
    }

    return FFaceSDFOperationResult::Success(
        SDFImages.Num());
}

FFaceSDFOperationResult
FFaceSDFPipeline::GenerateAtlasFromMesh(
    const FFaceSDFPipelineRequest& Request,
    FFaceSDFGrayImage& OutAtlas)
{
    OutAtlas.Reset();

    TArray<FFaceSDFGrayImage> Masks;
    TArray<FFaceSDFGrayImage> SDFImages;
    TArray<FFaceSDFLightSample> Samples;

    FFaceSDFOperationResult Result =
        GenerateShadowMasks(
            Request,
            Masks,
            Samples);

    if (!Result.bSucceeded)
    {
        return Result;
    }

    Result = ConvertMasksToSDF(
        Masks,
        SDFImages);

    if (!Result.bSucceeded)
    {
        return Result;
    }

    return BuildAtlas(
        SDFImages,
        Samples,
        OutAtlas);
}

