#include "FaceSDFImageIO.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

bool FFaceSDFImageIO::LoadGrayscalePNG(
	const FString& FilePath,
	FFaceSDFGrayImage& OutImage,
	FString& OutError
)
{
	OutImage.Reset();
	OutError.Reset();

	TArray<uint8> CompressedData;

    if (!FFileHelper::LoadFileToArray(
        CompressedData,
        *FilePath))
    {
        OutError = FString::Printf(
            TEXT("Failed to read PNG: %s"),
            *FilePath);

        return false;
    }//保护检查,是否能正常读取

    IImageWrapperModule& ImageWrapperModule =
        FModuleManager::LoadModuleChecked<
        IImageWrapperModule>(
            TEXT("ImageWrapper"));//调取ImageWrapper

    TSharedPtr<IImageWrapper> ImageWrapper =
        ImageWrapperModule.CreateImageWrapper(
            EImageFormat::PNG);//创建Imager Wrapper

    if (!ImageWrapper.IsValid())
    {
        OutError =
            TEXT("Failed to create PNG wrapper.");

        return false;
    }//保护

    if (!ImageWrapper->SetCompressed(
        CompressedData.GetData(),
        CompressedData.Num()))//二维解码
    {
        OutError = FString::Printf(
            TEXT("Failed to decode PNG: %s"),
            *FilePath);

        return false;
    }

    TArray64<uint8> RawData;
    if (!ImageWrapper->GetRaw(
        ERGBFormat::Gray,
        8,
        RawData))//图像提取
    {
        OutError = FString::Printf(
            TEXT("Failed to convert PNG to Gray8: %s"),
            *FilePath);

        return false;
    }

    OutImage.Width =
        ImageWrapper->GetWidth();

    OutImage.Height =
        ImageWrapper->GetHeight();

    OutImage.Pixels.Append(
        RawData.GetData(),
        RawData.Num());//图片像素整合
    

    if (!OutImage.IsValid())
    {
        OutImage.Reset();

        OutError = FString::Printf(
            TEXT("Invalid PNG pixel data: %s"),
            *FilePath);

        return false;
    }

    return true;
}

bool FFaceSDFImageIO::SaveGrayscalePNG(
    const FString& FilePath,
    const FFaceSDFGrayImage& Image,
    FString& OutError
)
{
    OutError.Reset();
    if (!Image.IsValid())
    {
        OutError =
            TEXT("Cannot save an invalid grayscale image.");

        return false;
    }//如果image没提取到直接报错
    

    IImageWrapperModule& ImageWrapperModule =
        FModuleManager::LoadModuleChecked<
        IImageWrapperModule>(
            TEXT("ImageWrapper"));

    TSharedPtr<IImageWrapper> ImageWrapper =
        ImageWrapperModule.CreateImageWrapper(
            EImageFormat::PNG);//依旧设定ImageWrapper

    if (!ImageWrapper.IsValid())
    {
        OutError =
            TEXT("Failed to create PNG wrapper.");

        return false;
    }//ImageWrapper的保护检查
    
    if (!ImageWrapper->SetRaw(
        Image.Pixels.GetData(),
        Image.Pixels.Num(),
        Image.Width,
        Image.Height,
        ERGBFormat::Gray,
        8))
    {
        OutError =
            TEXT("Failed to encode grayscale PNG.");

        return false;
    }//设定图片原始数据
    const TArray64<uint8>& CompressedData =
        ImageWrapper->GetCompressed(100);//GetCompressed压缩
    if (CompressedData.IsEmpty())
    {
        OutError =
            TEXT("PNG encoder returned empty data.");

        return false;
    }//GetCompressed安全检查

    const FString Directory =
        FPaths::GetPath(FilePath);

    if (!Directory.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(
            *Directory,
            true);
    }

    TArray<uint8> FileData;
    FileData.Append(
        CompressedData.GetData(),
        CompressedData.Num());//提取data

    if (!FFileHelper::SaveArrayToFile(
        FileData,
        *FilePath))//写入文件
    {
        OutError = FString::Printf(
            TEXT("Failed to save PNG: %s"),
            *FilePath);

        return false;
    }

    return true;

}