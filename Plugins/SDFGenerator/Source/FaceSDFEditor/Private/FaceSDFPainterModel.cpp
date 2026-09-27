#include "FaceSDFPainterModel.h"

bool FFaceSDFPainterModel::SetImage(
    const FFaceSDFGrayImage& InImage)
{
    if (!InImage.IsValid())
    {
        return false;
    }

    Image = InImage;
    return true;
}
bool FFaceSDFPainterModel::SetImage(
    FFaceSDFGrayImage&& InImage)
{
    if (!InImage.IsValid())
    {
        return false;
    }

    Image = MoveTemp(InImage);
    return true;
}
void FFaceSDFPainterModel::Reset()
{
    Image.Reset();
}

bool FFaceSDFPainterModel::HasValidImage() const
{
    return Image.IsValid();
}

const FFaceSDFGrayImage&
FFaceSDFPainterModel::GetImage() const
{
    return Image;
}

FFaceSDFGrayImage&
FFaceSDFPainterModel::GetMutableImage()
{
    return Image;
}

void FFaceSDFPainterModel::SetBrushRadius(
    float InRadius)
{
    BrushRadius =
        FMath::Clamp(
            InRadius,
            1.0f,
            512.0f);
}

float FFaceSDFPainterModel::GetBrushRadius() const
{
    return BrushRadius;
}

void FFaceSDFPainterModel::SetBrushValue(
    uint8 InValue)
{
    BrushValue = InValue;
}

uint8 FFaceSDFPainterModel::GetBrushValue() const
{
    return BrushValue;
}

bool FFaceSDFPainterModel::PaintAt(
    float ImageX,
    float ImageY
)
{
    if (!Image.IsValid())
    {
        return false;
    }

    const int32 CenterX = FMath::RoundToInt(ImageX);
    const int32 CenterY = FMath::RountToInt(ImageY);
    const int32 IntegerRadius = FMath::Max(
        1,
        FMath::CeilToInt(
            BrushRadius
        ));
    const float RadiusSquared =
        BrushRadius * BrushRadius;

    bool bModified = false;

    for (int32 OffsetY = -IntegerRadius;
        OffsetY <= IntegerRadius;
        ++OffsetY)
    {
        for (int32 OffsetX = -IntegerRadius;
            OffsetX <= IntegerRadius;
            ++OffsetX)
        {
            const float DistanceSquared =
                static_cast<float>(
                    OffsetX * OffsetX +
                    OffsetY * OffsetY);

            if (DistanceSquared >
                RadiusSquared)
            {
                continue;
            }

            const int32 X =
                CenterX + OffsetX;

            const int32 Y =
                CenterY + OffsetY;

            if (X < 0 ||
                X >= Image.Width ||
                Y < 0 ||
                Y >= Image.Height)
            {
                continue;
            }

            const int32 PixelIndex =
                Y * Image.Width + X;

            Image.Pixels[PixelIndex] =
                BrushValue;

            bModified = true;
        }
    }

    return bModified;
}

bool FFaceSDFPainterModel::Fill(
    uint8 Value)
{
    if (!Image.IsValid())
    {
        return false;
    }

    for (uint8& Pixel :
        Image.Pixels)
    {
        Pixel = Value;
    }

    return true;
}
