#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

class FFaceSDFPainterModel;
class UTexture2D;

class SFaceSDFPainter
	: public SLeafWidget
{
public: SLATE_BEGIN_ARGS(SFaceSDFPainter)
	:_Model(nullptr)
{
}

      SLATE_ARGUMENT(
          FFaceSDFPainterModel*,
          Model)

          SLATE_END_ARGS()

          void Construct(
              const FArguments & InArgs);

          void SetModel(
              FFaceSDFPainterModel * InModel);

          void RefreshPreview();

          virtual FVector2D ComputeDesiredSize(
              float LayoutScaleMultiplier) const override;

          virtual int32 OnPaint(
              const FPaintArgs & Args,
              const FGeometry & AllottedGeometry,
              const FSlateRect & MyCullingRect,
              FSlateWindowElementList & OutDrawElements,
              int32 LayerId,
              const FWidgetStyle & InWidgetStyle,
              bool bParentEnabled) const override;

          virtual FReply OnMouseButtonDown(
              const FGeometry & MyGeometry,
              const FPointerEvent & MouseEvent) override;

          virtual FReply OnMouseMove(
              const FGeometry & MyGeometry,
              const FPointerEvent & MouseEvent) override;

          virtual FReply OnMouseButtonUp(
              const FGeometry & MyGeometry,
              const FPointerEvent & MouseEvent) override;

private:
    void PaintAtMousePosition(
        const FGeometry & Geometry,
        const FVector2D & ScreenPosition);

private:
    FFaceSDFPainterModel* Model = nullptr;

    bool bIsDrawing = false;

    TStrongObjectPtr<UTexture2D>
        PreviewTexture;

    FSlateBrush PreviewBrush;
};