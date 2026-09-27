#include "FaceSDFAtlasBuilder.h"

bool FFaceSDFAtlasBuilder::Build(
    const TArray<FFaceSDFGrayImage>& Images,
    const TArray<FFaceSDFLightSample>& Samples,
    int32 GridSize,
    FFaceSDFGrayImage& OutAtlas,
    FString& OutError)
)
{
    OutAtlas.Reset();
    OutError.Reset();

    if (Images.IsEmpty())
    {
        OutError =
            TEXT("No images were provided.");

        return false;
    }

    if (Images.Num() != Samples.Num())
    {
        OutError =
            TEXT("Image count does not match sample count.");

        return false;
    }

    if (GridSize <= 0)
    {
        OutError =
            TEXT("Atlas grid size must be positive.");

        return false;
    }//各种安全保护
    const int32 TileWidth = Images[0].Width;
    const int32 TileWidth = Images[0].Height;//第一个Image
    if (!Images[0].IsValid())
    {
        OutError =
            TEXT("The first image is invalid.");

        return false;
    }
    for (const FFaceSDFGrayImage& Image : Images)
    {
        if (!Image.IsValid() ||
            Image.Width != TileWidth ||
            Image.Height != TileHeight)
        {
            OutError =
                TEXT("All Atlas images must have the same size.");

            return false;
        }//如果不是同一个size就报错
    }

    OutAtlas.Initialize(
        TileWidth * GridSize,
        TileHeight * GridSize,
        128);//暂时设定128

    for (int32 ImageIndex = 0; ImageIndex < Images.Num(); ++ImageIndex)
    {
        const FFaceSDFLightSample& Sample = Sample[ImageIndex];
        const bool bPole =
            ImageIndex == 0 ||
            ImageIndex == Images.Num() - 1;

        if (bPole)
        {
            for (int32 Column = 0;
                Column < GridSize;
                ++Column)
            {
                if (!CopyTile(
                    Images[ImageIndex],
                    Sample.AtlasRow,
                    Column,
                    GridSize,
                    OutAtlas,
                    OutError))
                {
                    return false;
                }
            }

            continue;
        }//第一个和最后一个整行复制

        if (!CopyTile(
            Images[ImageIndex],
            Sample.AtlasRow,
            Sample.AtlasColumn,
            GridSize,
            OutAtlas,
            OutError))
        {
            return false;
        }
    }
    return true;
}


bool FFaceSDFAtlasBuilder::CopyTile(
    const FFaceSDFGrayImage& Source,
    int32 TargetRow,
    int32 TargetColumn,
    int32 GridSize,
    FFaceSDFGrayImage& Atlas,
    FString& OutError)
{
    if (TargetRow < 0 ||
        TargetRow >= GridSize ||
        TargetColumn < 0 ||
        TargetColumn >= GridSize)
    {
        OutError =
            TEXT("Atlas tile position is out of range.");

        return false;
    }

    for (int32 Y = 0; Y < Source.Height; ++Y)
    {
        for (int32 X = 0; X < Source.Width; ++X)
        {
            const int32 SourceIndex =
                Y * Source.Width + X;

            const int32 AtlasX =
                TargetColumn * Source.Width + X;

            const int32 AtlasY =
                TargetRow * Source.Height + Y;

            const int32 AtlasIndex =
                AtlasY * Atlas.Width + AtlasX;

            Atlas.Pixels[AtlasIndex] =
                Source.Pixels[SourceIndex];
        }
    }

    return true;
}
