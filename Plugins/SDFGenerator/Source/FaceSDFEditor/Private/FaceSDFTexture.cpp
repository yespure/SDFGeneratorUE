#include "FaceSDFTexture.h"

#include "Engine/Texture2D.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"

#include "UObject/SavePackage.h"
#include "UObject/Package.h"


//Mask保存逻辑
bool FFaceSDFTexture::SaveFaceMaskTexture(
    const TArray<uint8>& Pixels,
    int32 Resolution,
    const FString& AssetName)
{
    if (Pixels.Num() != Resolution * Resolution)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid pixel data size."));
        return false;
    }

    FString SafeAssetName = AssetName;
    SafeAssetName = ObjectTools::SanitizeObjectName(SafeAssetName);//确保资源名称合法

    if (SafeAssetName.IsEmpty())
    {
        SafeAssetName = TEXT("FaceMask");
    }

    // 所有Shadow Mask都保存在同一个文件夹中
    const FString FolderPath =
        TEXT("/Game/FaceSDF/ShadowMasks");

    // PackageName同时表示资源保存路径和资源名称
    const FString PackageName =
        FolderPath +
        TEXT("/") +
        SafeAssetName;

    UPackage* Package =
        CreatePackage(*PackageName);//创建资源包

    if (!Package)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to create package."));
        return false;
    }

    UTexture2D* Texture = NewObject<UTexture2D>(
        Package,
        *SafeAssetName,
        RF_Public | RF_Standalone
    );//创建新的Texture2D对象

    if (!Texture)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to create texture."));
        return false;
    }

    Texture->Source.Init(
        Resolution,
        Resolution,
        1,
        1,
        TSF_G8);//初始化纹理源数据，设置为灰度图

    uint8* MipData =
        Texture->Source.LockMip(0);//锁定纹理数据以便写入

    if (!MipData)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to lock texture data."));
        return false;
    }

    FMemory::Memcpy(
        MipData,
        Pixels.GetData(),
        Pixels.Num());//将像素数据复制到纹理源数据中

    Texture->Source.UnlockMip(0);//解锁纹理数据

    Texture->SRGB = false;
    Texture->CompressionSettings = TC_Grayscale;
    Texture->MipGenSettings = TMGS_NoMipmaps;//纹理设置

    Texture->UpdateResource();//更新纹理资源

    FAssetRegistryModule::AssetCreated(Texture);

    Package->MarkPackageDirty();

    FString PackageFileName;

    if (!FPackageName::TryConvertLongPackageNameToFilename(
        PackageName,
        PackageFileName,
        FPackageName::GetAssetPackageExtension()))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to convert package name."));
        return false;
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

    const bool bSaved =
        UPackage::SavePackage(
            Package,
            Texture,
            *PackageFileName,
            SaveArgs);

    if (!bSaved)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to save texture package."));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Face SDF: Texture saved: %s"),
        *PackageName);

    return true;
}


//灰度图保存函数与上方保存函数同理,使用Chatgpt生成实现
bool FFaceSDFTexture::SaveFaceSDFTexture(
    const TArray<uint8>& Pixels,
    int32 Resolution,
    const FString& AssetName)
{
    if (Pixels.Num() != Resolution * Resolution)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Invalid SDF pixel data."));
        return false;
    }

    const FString FolderPath = TEXT("/Game/FaceSDF");

    const FString PackageName =
        FolderPath + TEXT("/") + AssetName;

    UPackage* Package =
        CreatePackage(*PackageName);

    if (!Package)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to create package."));
        return false;
    }

    UTexture2D* Texture =
        NewObject<UTexture2D>(
            Package,
            *AssetName,
            RF_Public | RF_Standalone);

    if (!Texture)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to create texture."));
        return false;
    }

    Texture->Source.Init(
        Resolution,
        Resolution,
        1,
        1,
        TSF_G8);

    uint8* MipData =
        Texture->Source.LockMip(0);

    if (!MipData)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to lock texture data."));
        return false;
    }

    FMemory::Memcpy(
        MipData,
        Pixels.GetData(),
        Pixels.Num());

    Texture->Source.UnlockMip(0);

    Texture->SRGB = false;
    Texture->CompressionSettings = TC_Grayscale;
    Texture->MipGenSettings = TMGS_NoMipmaps;
    Texture->Filter = TF_Bilinear;

    Texture->UpdateResource();

    FAssetRegistryModule::AssetCreated(Texture);

    Package->MarkPackageDirty();

    const FString PackageFileName =
        FPackageName::LongPackageNameToFilename(
            PackageName,
            FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

    const bool bSaved =
        UPackage::SavePackage(
            Package,
            Texture,
            *PackageFileName,
            SaveArgs);

    if (!bSaved)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to save texture."));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Face SDF: Grayscale SDF saved: %s"),
        *PackageName);

    return true;
}