#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"
//基本磁盘图片读写,这里主要处理PNG图片,而非texture资产
class FFaceSDFImageIO
{
public:
	static bool LoadGrayscalePNG(
		const FString& FilePath,
		FFAceSDFGrayImage& OutImage,
		FString& OutError
	);
	static bool SaveGrayscalePNG(
		const FString& FilePath,
		const FFaceSDFGrayImage& Image,
		FString& OutError);
};