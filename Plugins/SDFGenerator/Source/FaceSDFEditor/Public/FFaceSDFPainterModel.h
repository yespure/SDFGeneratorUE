#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"

class FFaceSDFPainterModel
{
public:
	bool SetImage(
		const FFaceSDFGrayImage& InImage
	);

	bool SetImage(
		FFaceSDFGrayImage&& InImage
	);

	void Reset();

    bool HasValidImage() const;

    const FFaceSDFGrayImage& GetImage() const;

    FFaceSDFGrayImage& GetMutableImage();

    void SetBrushRadius(
        float InRadius);

    float GetBrushRadius() const;

    void SetBrushValue(
        uint8 InValue);

    uint8 GetBrushValue() const;

    /**
     * ImageX¡¢ImageYÊÇÍ¼Æ¬ÏñËØ×ø±ê¡£
     */
    bool PaintAt(
        float ImageX,
        float ImageY);

    bool Fill(
        uint8 Value);

private:
    FFaceSDFGrayImage Image;

    float BrushRadius = 8.0f;

    uint8 BrushValue = 0;
};