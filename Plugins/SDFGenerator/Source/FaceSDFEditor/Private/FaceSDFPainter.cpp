#include "SFaceSDFPainter.h"

#include "FaceSDFPainterModel.h"

#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"

void SFaceSDFPainter::Construct(
    const FArguments& InArgs)
{
    Model = InArgs._Model;

    PreviewBrush.DrawAs =
        ESlateBrushDrawType::Image;

    RefreshPreview();
}

void SFaceSDFPainter::SetModel(
    FFaceSDFPainterModel* InModel)
{
    Model = InModel;
    RefreshPreview();
}

void SFaceSDFPainter::RefreshPreview()
{
    PreviewTexture.Reset();

    if (!Model ||
        !Model->HasValidImage())
    {
        PreviewBrush.SetResourceObject(
            nullptr);

        Invalidate(
            EInvalidateWidgetReason::Paint);

        return;
    }

    const FFaceSDFGrayImage& Image =
        Model->GetImage();

    UTexture2D* Texture =
        UTexture2D::CreateTransient(
            Image.Width,
            Image.Height,
            PF_B8G8R8A8);

    if (!Texture ||
        !Texture->GetPlatformData() ||
        Texture->GetPlatformData()
        ->Mips.IsEmpty())
    {
        return;
    }

    Texture->SRGB = false;
    Texture->Filter = TF_Nearest;
    Texture->NeverStream = true;

    FTexture2DMipMap& Mip =
        Texture->GetPlatformData()->Mips[0];

    void* RawData =
        Mip.BulkData.Lock(
            LOCK_READ_WRITE);

    if (!RawData)
    {
        Mip.BulkData.Unlock();
        return;
    }

    FColor* ColorPixels =
        static_cast<FColor*>(RawData);

    for (int32 PixelIndex = 0;
        PixelIndex < Image.Pixels.Num();
        ++PixelIndex)
    {
        const uint8 Value =
            Image.Pixels[PixelIndex];

        ColorPixels[PixelIndex] =
            FColor(
                Value,
                Value,
                Value,
                255);
    }

    Mip.BulkData.Unlock();

    Texture->UpdateResource();

    PreviewTexture =
        TStrongObjectPtr<UTexture2D>(
            Texture);

    PreviewBrush.SetResourceObject(
        Texture);

    PreviewBrush.ImageSize =
        FVector2D(
            Image.Width,
            Image.Height);

    Invalidate(
        EInvalidateWidgetReason::Paint);
}

FVector2D SFaceSDFPainter::ComputeDesiredSize(
    float LayoutScaleMultiplier) const
{
    return FVector2D(
        512.0f,
        512.0f);
}

int32 SFaceSDFPainter::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        &PreviewBrush,
        ESlateDrawEffect::None,
        FLinearColor::White);

    return LayerId;
}

FReply SFaceSDFPainter::OnMouseButtonDown(
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() !=
        EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    bIsDrawing = true;

    PaintAtMousePosition(
        MyGeometry,
        MouseEvent.GetScreenSpacePosition());

    return FReply::Handled()
        .CaptureMouse(
            SharedThis(this));
}

FReply SFaceSDFPainter::OnMouseMove(
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent)
{
    if (!bIsDrawing ||
        !HasMouseCapture())
    {
        return FReply::Unhandled();
    }

    PaintAtMousePosition(
        MyGeometry,
        MouseEvent.GetScreenSpacePosition());

    return FReply::Handled();
}

FReply SFaceSDFPainter::OnMouseButtonUp(
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() !=
        EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    bIsDrawing = false;

    return FReply::Handled()
        .ReleaseMouseCapture();
}

void SFaceSDFPainter::PaintAtMousePosition(
    const FGeometry& Geometry,
    const FVector2D& ScreenPosition)
{
    if (!Model ||
        !Model->HasValidImage())
    {
        return;
    }

    const FVector2D LocalPosition =
        Geometry.AbsoluteToLocal(
            ScreenPosition);

    const FVector2D LocalSize =
        Geometry.GetLocalSize();

    if (LocalSize.X <= 0.0f ||
        LocalSize.Y <= 0.0f)
    {
        return;
    }

    const FFaceSDFGrayImage& Image =
        Model->GetImage();

    const float NormalizedX =
        FMath::Clamp(
            LocalPosition.X / LocalSize.X,
            0.0f,
            1.0f);

    const float NormalizedY =
        FMath::Clamp(
            LocalPosition.Y / LocalSize.Y,
            0.0f,
            1.0f);

    const float ImageX =
        NormalizedX *
        static_cast<float>(
            Image.Width - 1);

    const float ImageY =
        NormalizedY *
        static_cast<float>(
            Image.Height - 1);

    // Model中的笔刷半径以图片像素为单位。
    if (Model->PaintAt(
        ImageX,
        ImageY))
    {
        RefreshPreview();
    }
}