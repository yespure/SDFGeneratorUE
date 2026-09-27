#pragma once

#include "CoreMinimal.h"

#include "FaceSDFPainterModel.h"
#include "FaceSDFPipeline.h"
#include "FaceSDFTypes.h"

#include "Widgets/SCompoundWidget.h"

class USkeletalMesh;
class SFaceSDFPainter;

struct FAssetData;

/**
 * Face SDF编辑器主窗口。
 *
 * 这个类只负责：
 * 1. 构建Slate界面；
 * 2. 保存用户输入的设置；
 * 3. 响应按钮；
 * 4. 调用Pipeline、ImageIO和Texture；
 * 5. 显示状态。
 *
 * 具体SDF算法、PNG编解码、Atlas拼接和Painter绘制算法
 * 都不应该放在这个类中。
 */
class SFaceSDFGeneratorWindow
    : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFaceSDFGeneratorWindow)
        {
        }
    SLATE_END_ARGS()

    void Construct(
        const FArguments& InArgs);

private:
    // =========================================================================
    // UI构建
    // =========================================================================

    TSharedRef<SWidget> BuildMeshSettingsPanel();

    TSharedRef<SWidget> BuildGenerationPanel();

    TSharedRef<SWidget> BuildPainterPanel();

    TSharedRef<SWidget> BuildStatusPanel();

    // =========================================================================
    // 模型设置
    // =========================================================================

    void OnMeshChanged(
        const FAssetData& AssetData);

    FString GetSelectedMeshAsset() const;

    TOptional<int32> GetLODValue() const;

    void OnLODChanged(
        int32 NewValue);

    TOptional<int32> GetSectionValue() const;

    void OnSectionChanged(
        int32 NewValue);

    TOptional<int32> GetUVChannelValue() const;

    void OnUVChannelChanged(
        int32 NewValue);

    TOptional<int32> GetResolutionValue() const;

    void OnResolutionChanged(
        int32 NewValue);

    FText GetOutputNameText() const;

    void OnOutputNameChanged(
        const FText& NewText);

    // =========================================================================
    // Shadow Mask / SDF / Atlas按钮
    // =========================================================================

    FReply OnGenerateShadowMasksClicked();

    FReply OnSelectShadowMasksClicked();

    FReply OnConvertMasksToSDFClicked();

    FReply OnSelectGrayscaleSDFsClicked();

    FReply OnGenerateAtlasClicked();

    FReply OnGenerateAtlasFromMeshClicked();

    FText GetShadowMaskStatusText() const;

    FText GetGrayscaleSDFStatusText() const;

    // =========================================================================
    // Painter按钮
    // =========================================================================

    FReply OnImportPainterImageClicked();

    FReply OnSavePainterImageClicked();

    FReply OnSetBlackBrushClicked();

    FReply OnSetWhiteBrushClicked();

    FReply OnClearPainterBlackClicked();

    FReply OnClearPainterWhiteClicked();

    TOptional<float> GetBrushRadius() const;

    void OnBrushRadiusChanged(
        float NewRadius);

    FText GetPainterStatusText() const;

    // =========================================================================
    // 辅助方法
    // =========================================================================

    bool BuildPipelineRequest(
        FFaceSDFPipelineRequest& OutRequest);

    bool OpenPNGFilesDialog(
        const FString& DialogTitle,
        bool bAllowMultiple,
        TArray<FString>& OutFiles);

    bool OpenSavePNGDialog(
        const FString& DefaultFileName,
        FString& OutFilePath);

    bool LoadImages(
        const TArray<FString>& FilePaths,
        TArray<FFaceSDFGrayImage>& OutImages);

    FString GetShadowMaskOutputDirectory() const;

    FString GetSDFOutputDirectory() const;

    FString GetAtlasOutputDirectory() const;

    FString MakeSDFFileName(
        const FString& ShadowMaskPath) const;

    FString GetSafeOutputName() const;

    void SetStatus(
        const FString& Message,
        bool bIsError = false);

private:
    // =========================================================================
    // 当前模型设置
    // =========================================================================

    TWeakObjectPtr<USkeletalMesh> SelectedMesh;

    int32 LODIndex = 0;

    int32 SectionIndex = 0;

    int32 UVChannel = 0;

    int32 Resolution = 128;

    FString OutputName =
        TEXT("FaceSDF_Atlas");

    // =========================================================================
    // 当前选择的文件
    // =========================================================================

    TArray<FString> SelectedShadowMaskFiles;

    TArray<FString> SelectedGrayscaleSDFFiles;

    // =========================================================================
    // Painter
    // =========================================================================

    FFaceSDFPainterModel PainterModel;

    TSharedPtr<SFaceSDFPainter> PainterWidget;

    FString PainterSourcePath;

    // =========================================================================
    // 状态显示
    // =========================================================================

    FString StatusMessage =
        TEXT("Ready.");

    bool bLastStatusWasError = false;
};