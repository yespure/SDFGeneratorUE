#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/WeakObjectPtr.h"
#include "FaceSDFGenerator.h"

class USkeletalMesh;
struct FAssetData;

class SFaceSDFGeneratorWindow : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SFaceSDFGeneratorWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    // 按钮点击
    FReply OnGenerateClicked();

    // 资源选择
    void OnMeshChanged(const FAssetData& AssetData);
    FString GetSelectedMeshAsset() const;

    // ===== LOD =====
    TOptional<int32> GetLODValue() const;
    void OnLODChanged(int32 NewValue);

    // ===== Section =====
    TOptional<int32> GetSectionValue() const;
    void OnSectionChanged(int32 NewValue);

    // ===== UVChannel =====
    TOptional<int32> GetUVChannelValue() const;
    void OnUVChannelChanged(int32 NewValue);

    // ===== Resolution =====
    TOptional<int32> GetResolutionValue() const;
    void OnResolutionChanged(int32 NewValue);

    // ===== OutputName =====
    FText GetOutputNameText() const;
    void OnOutputNameChanged(const FText& NewText);

    // ===== Shadow Mask 批量导入 =====
    FReply OnSelectShadowMasksClicked();
    FReply OnGenerateAllSDFClicked();
    FText GetShadowMaskStatusText() const;

    bool ReadShadowMaskPNG(
        const FString& FilePath,
        TArray<uint8>& OutPixels,
        int32& OutWidth,
        int32& OutHeight);

    // SDF图集生成
    FReply OnGenerateAtlasClicked();

    bool GenerateSDFAtlas(
        const TArray<FFaceSDFTriangle>& FaceTriangles,
        int32 SDFResolution,
        TArray<uint8>& OutAtlasPixels,
        int32& OutAtlasResolution);

    bool LoadPNGAsGrayscale(
        const FString& FilePath,
        TArray<uint8>& OutPixels,
        int32& OutWidth,
        int32& OutHeight);

    bool SaveSDFAtlasTexture(
        const TArray<uint8>& Pixels,
        int32 AtlasResolution,
        const FString& AssetName);

    TArray<FString> SelectedShadowMaskFiles;

    int32 LODIndex = 0;
    int32 SectionIndex = 0;
    int32 UVChannel = 0;
    int32 Resolution = 128;

    FString OutputName = TEXT("FaceSDF");

    TWeakObjectPtr<USkeletalMesh> SelectedMesh;
};