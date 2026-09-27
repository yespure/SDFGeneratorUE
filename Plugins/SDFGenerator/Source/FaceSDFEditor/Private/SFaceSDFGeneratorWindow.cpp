#include "SFaceSDFGeneratorWindow.h"

#include "FaceSDFImageIO.h"
#include "FaceSDFLightSampler.h"
#include "FaceSDFTexture.h"
#include "SFaceSDFPainter.h"

#include "AssetRegistry/AssetData.h"
#include "DesktopPlatformModule.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "PropertyCustomizationHelpers.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SFaceSDFGeneratorWindow"

namespace FaceSDFWindowConstants
{
    static constexpr int32 AtlasGridSize = 9;

    static constexpr int32 ExpectedSampleCount = 65;

    static constexpr int32 MinimumResolution = 16;

    static constexpr int32 MaximumResolution = 512;

    static constexpr float PainterPreviewSize = 512.0f;
}

// =============================================================================
// 窗口构建
// =============================================================================

void SFaceSDFGeneratorWindow::Construct(
    const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SScrollBox)

                + SScrollBox::Slot()
                [
                    SNew(SVerticalBox)

                        // -----------------------------------------------------------------
                        // 标题
                        // -----------------------------------------------------------------

                        +SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(10.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "WindowTitle",
                                    "Face SDF Generator"))
                        ]

                        // -----------------------------------------------------------------
                        // 模型设置
                        // -----------------------------------------------------------------

                        +SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(8.0f)
                        [
                            BuildMeshSettingsPanel()
                        ]

                        // -----------------------------------------------------------------
                        // 生成流程
                        // -----------------------------------------------------------------

                        +SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(8.0f)
                        [
                            BuildGenerationPanel()
                        ]

                        // -----------------------------------------------------------------
                        // Painter
                        // -----------------------------------------------------------------

                        +SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(8.0f)
                        [
                            BuildPainterPanel()
                        ]

                        // -----------------------------------------------------------------
                        // 状态
                        // -----------------------------------------------------------------

                        +SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(8.0f)
                        [
                            BuildStatusPanel()
                        ]
                ]
        ];
}

// =============================================================================
// 模型设置UI
// =============================================================================

TSharedRef<SWidget>
SFaceSDFGeneratorWindow::BuildMeshSettingsPanel()
{
    return
        SNew(SBorder)
        .Padding(8.0f)
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(STextBlock)
                        .Text(LOCTEXT(
                            "MeshSettingsTitle",
                            "Mesh Settings"))
                ]

                // -----------------------------------------------------------------
                // Skeletal Mesh
                // -----------------------------------------------------------------

                +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "SelectMeshLabel",
                                    "Skeletal Mesh"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SObjectPropertyEntryBox)
                                .AllowedClass(
                                    USkeletalMesh::StaticClass())
                                .ObjectPath(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetSelectedMeshAsset)
                                .OnObjectChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnMeshChanged)
                        ]
                ]

            // -----------------------------------------------------------------
            // LOD
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "LODLabel",
                                    "LOD"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetLODValue)
                                .OnValueChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnLODChanged)
                                .MinValue(0)
                                .MinSliderValue(0)
                        ]
                ]

            // -----------------------------------------------------------------
            // Section
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "SectionLabel",
                                    "Section"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetSectionValue)
                                .OnValueChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnSectionChanged)
                                .MinValue(0)
                                .MinSliderValue(0)
                        ]
                ]

            // -----------------------------------------------------------------
            // UV Channel
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "UVChannelLabel",
                                    "UV Channel"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetUVChannelValue)
                                .OnValueChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnUVChannelChanged)
                                .MinValue(0)
                                .MinSliderValue(0)
                        ]
                ]

            // -----------------------------------------------------------------
            // Resolution
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "ResolutionLabel",
                                    "Resolution"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetResolutionValue)
                                .OnValueChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnResolutionChanged)
                                .MinValue(
                                    FaceSDFWindowConstants::
                                    MinimumResolution)
                                .MaxValue(
                                    FaceSDFWindowConstants::
                                    MaximumResolution)
                                .MinSliderValue(
                                    FaceSDFWindowConstants::
                                    MinimumResolution)
                                .MaxSliderValue(
                                    FaceSDFWindowConstants::
                                    MaximumResolution)
                        ]
                ]

            // -----------------------------------------------------------------
            // Output Name
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "OutputNameLabel",
                                    "Output Name"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SEditableTextBox)
                                .Text(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetOutputNameText)
                                .OnTextChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnOutputNameChanged)
                        ]
                ]
        ];
}

// =============================================================================
// 生成流程UI
// =============================================================================

TSharedRef<SWidget>
SFaceSDFGeneratorWindow::BuildGenerationPanel()
{
    return
        SNew(SBorder)
        .Padding(8.0f)
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(STextBlock)
                        .Text(LOCTEXT(
                            "GenerationTitle",
                            "SDF Generation"))
                ]

                // -----------------------------------------------------------------
                // 生成Shadow Mask
                // -----------------------------------------------------------------

                +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "GenerateShadowMasksButton",
                            "Generate Shadow Mask PNGs"))
                        .ToolTipText(LOCTEXT(
                            "GenerateShadowMasksTooltip",
                            "Generate 65 Shadow Mask PNGs from the selected mesh."))
                        .OnClicked(
                            this,
                            &SFaceSDFGeneratorWindow::
                            OnGenerateShadowMasksClicked)
                ]

            // -----------------------------------------------------------------
            // 选择Shadow Mask
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "SelectShadowMasksButton",
                            "Select Shadow Mask PNGs"))
                        .OnClicked(
                            this,
                            &SFaceSDFGeneratorWindow::
                            OnSelectShadowMasksClicked)
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(STextBlock)
                        .Text(
                            this,
                            &SFaceSDFGeneratorWindow::
                            GetShadowMaskStatusText)
                ]

            // -----------------------------------------------------------------
            // Mask转SDF
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "ConvertMasksButton",
                            "Convert Shadow Masks to Grayscale SDF PNGs"))
                        .OnClicked(
                            this,
                            &SFaceSDFGeneratorWindow::
                            OnConvertMasksToSDFClicked)
                ]

            // -----------------------------------------------------------------
            // 选择SDF
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "SelectSDFsButton",
                            "Select Grayscale SDF PNGs"))
                        .OnClicked(
                            this,
                            &SFaceSDFGeneratorWindow::
                            OnSelectGrayscaleSDFsClicked)
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(STextBlock)
                        .Text(
                            this,
                            &SFaceSDFGeneratorWindow::
                            GetGrayscaleSDFStatusText)
                ]

            // -----------------------------------------------------------------
            // 从选中的SDF生成Atlas
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "GenerateAtlasButton",
                            "Generate Atlas from Selected SDF PNGs"))
                        .OnClicked(
                            this,
                            &SFaceSDFGeneratorWindow::
                            OnGenerateAtlasClicked)
                ]

            // -----------------------------------------------------------------
            // 一步生成
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "GenerateDirectAtlasButton",
                            "Generate Atlas Directly from Mesh"))
                        .ToolTipText(LOCTEXT(
                            "GenerateDirectAtlasTooltip",
                            "Generate Shadow Masks, SDF images and Atlas in one operation."))
                        .OnClicked(
                            this,
                            &SFaceSDFGeneratorWindow::
                            OnGenerateAtlasFromMeshClicked)
                ]
        ];
}

// =============================================================================
// Painter UI
// =============================================================================

TSharedRef<SWidget>
SFaceSDFGeneratorWindow::BuildPainterPanel()
{
    return
        SNew(SBorder)
        .Padding(8.0f)
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(STextBlock)
                        .Text(LOCTEXT(
                            "PainterTitle",
                            "Black and White Painter"))
                ]

                // -----------------------------------------------------------------
                // 导入和保存
                // -----------------------------------------------------------------

                +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT(
                                    "ImportPainterImageButton",
                                    "Import PNG"))
                                .OnClicked(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnImportPainterImageClicked)
                        ]

                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT(
                                    "SavePainterImageButton",
                                    "Save PNG"))
                                .OnClicked(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnSavePainterImageClicked)
                        ]
                ]

            // -----------------------------------------------------------------
            // 黑白笔刷
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT(
                                    "BlackBrushButton",
                                    "Black Brush"))
                                .OnClicked(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnSetBlackBrushClicked)
                        ]

                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT(
                                    "WhiteBrushButton",
                                    "White Brush"))
                                .OnClicked(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnSetWhiteBrushClicked)
                        ]

                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(8.0f, 0.0f)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT(
                                    "BrushRadiusLabel",
                                    "Brush Radius"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(2.0f)
                        [
                            SNew(SNumericEntryBox<float>)
                                .Value(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    GetBrushRadius)
                                .OnValueChanged(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnBrushRadiusChanged)
                                .MinValue(1.0f)
                                .MaxValue(512.0f)
                                .MinSliderValue(1.0f)
                                .MaxSliderValue(128.0f)
                        ]
                ]

            // -----------------------------------------------------------------
            // 清空
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT(
                                    "ClearBlackButton",
                                    "Clear Black"))
                                .OnClicked(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnClearPainterBlackClicked)
                        ]

                    + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2.0f)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT(
                                    "ClearWhiteButton",
                                    "Clear White"))
                                .OnClicked(
                                    this,
                                    &SFaceSDFGeneratorWindow::
                                    OnClearPainterWhiteClicked)
                        ]
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(STextBlock)
                        .Text(
                            this,
                            &SFaceSDFGeneratorWindow::
                            GetPainterStatusText)
                ]

            // -----------------------------------------------------------------
            // Painter画布
            // -----------------------------------------------------------------

            +SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(4.0f)
                [
                    SNew(SBox)
                        .WidthOverride(
                            FaceSDFWindowConstants::
                            PainterPreviewSize)
                        .HeightOverride(
                            FaceSDFWindowConstants::
                            PainterPreviewSize)
                        [
                            SAssignNew(
                                PainterWidget,
                                SFaceSDFPainter)
                                .Model(&PainterModel)
                        ]
                ]
        ];
}

// =============================================================================
// 状态UI
// =============================================================================

TSharedRef<SWidget>
SFaceSDFGeneratorWindow::BuildStatusPanel()
{
    return
        SNew(SBorder)
        .Padding(8.0f)
        [
            SNew(STextBlock)
                .Text_Lambda(
                    [this]()
                    {
                        return FText::FromString(
                            StatusMessage);
                    })
                .ColorAndOpacity_Lambda(
                    [this]()
                    {
                        if (bLastStatusWasError)
                        {
                            return FSlateColor(
                                FLinearColor::Red);
                        }

                        return FSlateColor(
                            FLinearColor::White);
                    })
        ];
}

// =============================================================================
// 模型设置
// =============================================================================

void SFaceSDFGeneratorWindow::OnMeshChanged(
    const FAssetData& AssetData)
{
    SelectedMesh =
        Cast<USkeletalMesh>(
            AssetData.GetAsset());

    if (SelectedMesh.IsValid())
    {
        SetStatus(
            FString::Printf(
                TEXT("Selected mesh: %s"),
                *SelectedMesh->GetName()));
    }
    else
    {
        SetStatus(
            TEXT("No mesh selected."));
    }
}

FString
SFaceSDFGeneratorWindow::GetSelectedMeshAsset() const
{
    if (SelectedMesh.IsValid())
    {
        return SelectedMesh->GetPathName();
    }

    return FString();
}

TOptional<int32>
SFaceSDFGeneratorWindow::GetLODValue() const
{
    return LODIndex;
}

void SFaceSDFGeneratorWindow::OnLODChanged(
    int32 NewValue)
{
    LODIndex =
        FMath::Max(
            0,
            NewValue);
}

TOptional<int32>
SFaceSDFGeneratorWindow::GetSectionValue() const
{
    return SectionIndex;
}

void SFaceSDFGeneratorWindow::OnSectionChanged(
    int32 NewValue)
{
    SectionIndex =
        FMath::Max(
            0,
            NewValue);
}

TOptional<int32>
SFaceSDFGeneratorWindow::GetUVChannelValue() const
{
    return UVChannel;
}

void SFaceSDFGeneratorWindow::OnUVChannelChanged(
    int32 NewValue)
{
    UVChannel =
        FMath::Max(
            0,
            NewValue);
}

TOptional<int32>
SFaceSDFGeneratorWindow::GetResolutionValue() const
{
    return Resolution;
}

void SFaceSDFGeneratorWindow::OnResolutionChanged(
    int32 NewValue)
{
    Resolution =
        FMath::Clamp(
            NewValue,
            FaceSDFWindowConstants::
            MinimumResolution,
            FaceSDFWindowConstants::
            MaximumResolution);
}

FText
SFaceSDFGeneratorWindow::GetOutputNameText() const
{
    return FText::FromString(
        OutputName);
}

void SFaceSDFGeneratorWindow::OnOutputNameChanged(
    const FText& NewText)
{
    OutputName =
        NewText.ToString();
}

// =============================================================================
// Pipeline Request
// =============================================================================

bool SFaceSDFGeneratorWindow::BuildPipelineRequest(
    FFaceSDFPipelineRequest& OutRequest)
{
    if (!SelectedMesh.IsValid())
    {
        SetStatus(
            TEXT("Please select a Skeletal Mesh."),
            true);

        return false;
    }

    OutRequest.Mesh =
        SelectedMesh.Get();

    OutRequest.MeshSettings.LODIndex =
        LODIndex;

    OutRequest.MeshSettings.SectionIndex =
        SectionIndex;

    OutRequest.MeshSettings.UVChannel =
        UVChannel;

    OutRequest.GenerationSettings.Resolution =
        Resolution;

    OutRequest.GenerationSettings.AtlasGridSize =
        FaceSDFWindowConstants::
        AtlasGridSize;

    return true;
}

// =============================================================================
// 生成Shadow Mask
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnGenerateShadowMasksClicked()
{
    FFaceSDFPipelineRequest Request;

    if (!BuildPipelineRequest(Request))
    {
        return FReply::Handled();
    }

    TArray<FFaceSDFGrayImage> Masks;

    TArray<FFaceSDFLightSample> Samples;

    const FFaceSDFOperationResult Result =
        FFaceSDFPipeline::GenerateShadowMasks(
            Request,
            Masks,
            Samples);

    if (!Result.bSucceeded)
    {
        SetStatus(
            Result.ErrorMessage,
            true);

        return FReply::Handled();
    }

    const FString OutputDirectory =
        GetShadowMaskOutputDirectory();

    IFileManager::Get().MakeDirectory(
        *OutputDirectory,
        true);

    int32 SavedCount = 0;

    for (int32 ImageIndex = 0;
        ImageIndex < Masks.Num();
        ++ImageIndex)
    {
        if (!Samples.IsValidIndex(
            ImageIndex))
        {
            SetStatus(
                TEXT(
                    "Light sample count does not match mask count."),
                true);

            return FReply::Handled();
        }

        const FString FilePath =
            FPaths::Combine(
                OutputDirectory,
                Samples[ImageIndex]
                .OutputName +
                TEXT(".png"));

        FString Error;

        if (!FFaceSDFImageIO::SaveGrayscalePNG(
            FilePath,
            Masks[ImageIndex],
            Error))
        {
            SetStatus(
                Error,
                true);

            return FReply::Handled();
        }

        ++SavedCount;
    }

    SetStatus(
        FString::Printf(
            TEXT(
                "Generated %d Shadow Mask PNGs: %s"),
            SavedCount,
            *OutputDirectory));

    return FReply::Handled();
}

// =============================================================================
// 选择Shadow Mask
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnSelectShadowMasksClicked()
{
    TArray<FString> Files;

    if (!OpenPNGFilesDialog(
        TEXT("Select Shadow Mask PNGs"),
        true,
        Files))
    {
        return FReply::Handled();
    }

    Files.Sort();

    SelectedShadowMaskFiles =
        MoveTemp(Files);

    SetStatus(
        FString::Printf(
            TEXT(
                "Selected %d Shadow Mask PNGs."),
            SelectedShadowMaskFiles.Num()));

    return FReply::Handled();
}

// =============================================================================
// Shadow Mask转SDF
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnConvertMasksToSDFClicked()
{
    if (SelectedShadowMaskFiles.IsEmpty())
    {
        SetStatus(
            TEXT(
                "No Shadow Mask PNGs are selected."),
            true);

        return FReply::Handled();
    }

    TArray<FFaceSDFGrayImage> Masks;

    if (!LoadImages(
        SelectedShadowMaskFiles,
        Masks))
    {
        return FReply::Handled();
    }

    TArray<FFaceSDFGrayImage> SDFImages;

    const FFaceSDFOperationResult Result =
        FFaceSDFPipeline::ConvertMasksToSDF(
            Masks,
            SDFImages);

    if (!Result.bSucceeded)
    {
        SetStatus(
            Result.ErrorMessage,
            true);

        return FReply::Handled();
    }

    const FString OutputDirectory =
        GetSDFOutputDirectory();

    IFileManager::Get().MakeDirectory(
        *OutputDirectory,
        true);

    int32 SavedCount = 0;

    for (int32 ImageIndex = 0;
        ImageIndex < SDFImages.Num();
        ++ImageIndex)
    {
        if (!SelectedShadowMaskFiles.IsValidIndex(
            ImageIndex))
        {
            SetStatus(
                TEXT(
                    "Input file count does not match SDF image count."),
                true);

            return FReply::Handled();
        }

        const FString FileName =
            MakeSDFFileName(
                SelectedShadowMaskFiles[
                    ImageIndex]);

        const FString FilePath =
            FPaths::Combine(
                OutputDirectory,
                FileName);

        FString Error;

        if (!FFaceSDFImageIO::SaveGrayscalePNG(
            FilePath,
            SDFImages[ImageIndex],
            Error))
        {
            SetStatus(
                Error,
                true);

            return FReply::Handled();
        }

        ++SavedCount;
    }

    SetStatus(
        FString::Printf(
            TEXT(
                "Converted %d Shadow Masks to SDF PNGs: %s"),
            SavedCount,
            *OutputDirectory));

    return FReply::Handled();
}

// =============================================================================
// 选择SDF
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnSelectGrayscaleSDFsClicked()
{
    TArray<FString> Files;

    if (!OpenPNGFilesDialog(
        TEXT("Select Grayscale SDF PNGs"),
        true,
        Files))
    {
        return FReply::Handled();
    }

    Files.Sort();

    SelectedGrayscaleSDFFiles =
        MoveTemp(Files);

    SetStatus(
        FString::Printf(
            TEXT(
                "Selected %d Grayscale SDF PNGs."),
            SelectedGrayscaleSDFFiles.Num()));

    return FReply::Handled();
}

// =============================================================================
// 通过65张SDF生成Atlas
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnGenerateAtlasClicked()
{
    if (SelectedGrayscaleSDFFiles.Num() !=
        FaceSDFWindowConstants::
        ExpectedSampleCount)
    {
        SetStatus(
            FString::Printf(
                TEXT(
                    "Atlas requires exactly 65 SDF PNGs. Current: %d"),
                SelectedGrayscaleSDFFiles.Num()),
            true);

        return FReply::Handled();
    }

    TArray<FFaceSDFGrayImage> SDFImages;

    if (!LoadImages(
        SelectedGrayscaleSDFFiles,
        SDFImages))
    {
        return FReply::Handled();
    }

    TArray<FFaceSDFLightSample> Samples;

    FFaceSDFLightSampler::BuildDefaultSamples(
        Samples);

    FFaceSDFGrayImage Atlas;

    const FFaceSDFOperationResult Result =
        FFaceSDFPipeline::BuildAtlas(
            SDFImages,
            Samples,
            Atlas);

    if (!Result.bSucceeded)
    {
        SetStatus(
            Result.ErrorMessage,
            true);

        return FReply::Handled();
    }

    const FString SafeOutputName =
        GetSafeOutputName();

    const FString PNGPath =
        FPaths::Combine(
            GetAtlasOutputDirectory(),
            SafeOutputName +
            TEXT(".png"));

    FString Error;

    if (!FFaceSDFImageIO::SaveGrayscalePNG(
        PNGPath,
        Atlas,
        Error))
    {
        SetStatus(
            Error,
            true);

        return FReply::Handled();
    }

    if (!FFaceSDFTexture::SaveGrayscaleTexture(
        Atlas,
        TEXT("/Game/FaceSDF/Atlas"),
        SafeOutputName,
        Error))
    {
        SetStatus(
            Error,
            true);

        return FReply::Handled();
    }

    SetStatus(
        FString::Printf(
            TEXT(
                "Atlas generated successfully: %s"),
            *PNGPath));

    return FReply::Handled();
}

// =============================================================================
// 从模型一步生成Atlas
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnGenerateAtlasFromMeshClicked()
{
    FFaceSDFPipelineRequest Request;

    if (!BuildPipelineRequest(
        Request))
    {
        return FReply::Handled();
    }

    FFaceSDFGrayImage Atlas;

    const FFaceSDFOperationResult Result =
        FFaceSDFPipeline::GenerateAtlasFromMesh(
            Request,
            Atlas);

    if (!Result.bSucceeded)
    {
        SetStatus(
            Result.ErrorMessage,
            true);

        return FReply::Handled();
    }

    const FString SafeOutputName =
        GetSafeOutputName();

    const FString PNGPath =
        FPaths::Combine(
            GetAtlasOutputDirectory(),
            SafeOutputName +
            TEXT(".png"));

    FString Error;

    if (!FFaceSDFImageIO::SaveGrayscalePNG(
        PNGPath,
        Atlas,
        Error))
    {
        SetStatus(
            Error,
            true);

        return FReply::Handled();
    }

    if (!FFaceSDFTexture::SaveGrayscaleTexture(
        Atlas,
        TEXT("/Game/FaceSDF/Atlas"),
        SafeOutputName,
        Error))
    {
        SetStatus(
            Error,
            true);

        return FReply::Handled();
    }

    SetStatus(
        FString::Printf(
            TEXT(
                "Atlas generated directly from mesh: %s"),
            *PNGPath));

    return FReply::Handled();
}

// =============================================================================
// 文件选择状态
// =============================================================================

FText
SFaceSDFGeneratorWindow::GetShadowMaskStatusText() const
{
    return FText::Format(
        LOCTEXT(
            "ShadowMaskStatus",
            "{0} Shadow Mask PNGs selected"),
        FText::AsNumber(
            SelectedShadowMaskFiles.Num()));
}

FText
SFaceSDFGeneratorWindow::GetGrayscaleSDFStatusText() const
{
    return FText::Format(
        LOCTEXT(
            "GrayscaleSDFStatus",
            "{0} Grayscale SDF PNGs selected"),
        FText::AsNumber(
            SelectedGrayscaleSDFFiles.Num()));
}

// =============================================================================
// Painter导入
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnImportPainterImageClicked()
{
    TArray<FString> Files;

    if (!OpenPNGFilesDialog(
        TEXT("Import PNG into Painter"),
        false,
        Files))
    {
        return FReply::Handled();
    }

    if (Files.IsEmpty())
    {
        return FReply::Handled();
    }

    FFaceSDFGrayImage ImportedImage;

    FString Error;

    if (!FFaceSDFImageIO::LoadGrayscalePNG(
        Files[0],
        ImportedImage,
        Error))
    {
        SetStatus(
            Error,
            true);

        return FReply::Handled();
    }

    if (!PainterModel.SetImage(
        MoveTemp(ImportedImage)))
    {
        SetStatus(
            TEXT(
                "Failed to assign the imported image to Painter."),
            true);

        return FReply::Handled();
    }

    PainterSourcePath =
        Files[0];

    if (PainterWidget.IsValid())
    {
        PainterWidget->RefreshPreview();
    }

    const FFaceSDFGrayImage& Image =
        PainterModel.GetImage();

    SetStatus(
        FString::Printf(
            TEXT(
                "Painter loaded: %s (%d x %d)"),
            *FPaths::GetCleanFilename(
                PainterSourcePath),
            Image.Width,
            Image.Height));

    return FReply::Handled();
}

// =============================================================================
// Painter保存
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnSavePainterImageClicked()
{
    if (!PainterModel.HasValidImage())
    {
        SetStatus(
            TEXT(
                "Import an image into Painter first."),
            true);

        return FReply::Handled();
    }

    FString DefaultFileName =
        PainterSourcePath.IsEmpty()
        ? TEXT("PaintedMask.png")
        : FPaths::GetCleanFilename(
            PainterSourcePath);

    FString SavePath;

    if (!OpenSavePNGDialog(
        DefaultFileName,
        SavePath))
    {
        return FReply::Handled();
    }

    FString Error;

    if (!FFaceSDFImageIO::SaveGrayscalePNG(
        SavePath,
        PainterModel.GetImage(),
        Error))
    {
        SetStatus(
            Error,
            true);

        return FReply::Handled();
    }

    SetStatus(
        FString::Printf(
            TEXT(
                "Painter image saved: %s"),
            *SavePath));

    return FReply::Handled();
}

// =============================================================================
// Painter笔刷
// =============================================================================

FReply
SFaceSDFGeneratorWindow::OnSetBlackBrushClicked()
{
    PainterModel.SetBrushValue(0);

    SetStatus(
        TEXT("Painter brush set to black."));

    return FReply::Handled();
}

FReply
SFaceSDFGeneratorWindow::OnSetWhiteBrushClicked()
{
    PainterModel.SetBrushValue(255);

    SetStatus(
        TEXT("Painter brush set to white."));

    return FReply::Handled();
}

FReply
SFaceSDFGeneratorWindow::OnClearPainterBlackClicked()
{
    if (!PainterModel.Fill(0))
    {
        SetStatus(
            TEXT(
                "Import an image into Painter first."),
            true);

        return FReply::Handled();
    }

    if (PainterWidget.IsValid())
    {
        PainterWidget->RefreshPreview();
    }

    SetStatus(
        TEXT("Painter image cleared to black."));

    return FReply::Handled();
}

FReply
SFaceSDFGeneratorWindow::OnClearPainterWhiteClicked()
{
    if (!PainterModel.Fill(255))
    {
        SetStatus(
            TEXT(
                "Import an image into Painter first."),
            true);

        return FReply::Handled();
    }

    if (PainterWidget.IsValid())
    {
        PainterWidget->RefreshPreview();
    }

    SetStatus(
        TEXT("Painter image cleared to white."));

    return FReply::Handled();
}

TOptional<float>
SFaceSDFGeneratorWindow::GetBrushRadius() const
{
    return PainterModel.GetBrushRadius();
}

void SFaceSDFGeneratorWindow::OnBrushRadiusChanged(
    float NewRadius)
{
    PainterModel.SetBrushRadius(
        NewRadius);
}

FText
SFaceSDFGeneratorWindow::GetPainterStatusText() const
{
    if (!PainterModel.HasValidImage())
    {
        return LOCTEXT(
            "PainterNoImage",
            "No image loaded.");
    }

    const FFaceSDFGrayImage& Image =
        PainterModel.GetImage();

    return FText::Format(
        LOCTEXT(
            "PainterImageStatus",
            "Image size: {0} x {1}, Brush radius: {2}"),
        FText::AsNumber(Image.Width),
        FText::AsNumber(Image.Height),
        FText::AsNumber(
            PainterModel.GetBrushRadius()));
}

// =============================================================================
// 文件对话框
// =============================================================================

bool SFaceSDFGeneratorWindow::OpenPNGFilesDialog(
    const FString& DialogTitle,
    bool bAllowMultiple,
    TArray<FString>& OutFiles)
{
    OutFiles.Reset();

    IDesktopPlatform* DesktopPlatform =
        FDesktopPlatformModule::Get();

    if (!DesktopPlatform)
    {
        SetStatus(
            TEXT(
                "Desktop Platform module is unavailable."),
            true);

        return false;
    }

    const void* ParentWindowHandle =
        FSlateApplication::Get()
        .FindBestParentWindowHandleForDialogs(
            nullptr);

    const uint32 DialogFlags =
        bAllowMultiple
        ? static_cast<uint32>(
            EFileDialogFlags::Multiple)
        : static_cast<uint32>(
            EFileDialogFlags::None);

    return DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        DialogTitle,
        FPaths::ProjectDir(),
        TEXT(""),
        TEXT("PNG Images (*.png)|*.png"),
        DialogFlags,
        OutFiles);
}

bool SFaceSDFGeneratorWindow::OpenSavePNGDialog(
    const FString& DefaultFileName,
    FString& OutFilePath)
{
    OutFilePath.Reset();

    IDesktopPlatform* DesktopPlatform =
        FDesktopPlatformModule::Get();

    if (!DesktopPlatform)
    {
        SetStatus(
            TEXT(
                "Desktop Platform module is unavailable."),
            true);

        return false;
    }

    const void* ParentWindowHandle =
        FSlateApplication::Get()
        .FindBestParentWindowHandleForDialogs(
            nullptr);

    TArray<FString> SaveFiles;

    const bool bAccepted =
        DesktopPlatform->SaveFileDialog(
            ParentWindowHandle,
            TEXT("Save PNG"),
            FPaths::ProjectSavedDir(),
            DefaultFileName,
            TEXT("PNG Images (*.png)|*.png"),
            EFileDialogFlags::None,
            SaveFiles);

    if (!bAccepted ||
        SaveFiles.IsEmpty())
    {
        return false;
    }

    OutFilePath =
        SaveFiles[0];

    if (!OutFilePath.EndsWith(
        TEXT(".png"),
        ESearchCase::IgnoreCase))
    {
        OutFilePath +=
            TEXT(".png");
    }

    return true;
}

// =============================================================================
// 批量加载图片
// =============================================================================

bool SFaceSDFGeneratorWindow::LoadImages(
    const TArray<FString>& FilePaths,
    TArray<FFaceSDFGrayImage>& OutImages)
{
    OutImages.Reset();

    OutImages.Reserve(
        FilePaths.Num());

    for (const FString& FilePath :
        FilePaths)
    {
        FFaceSDFGrayImage Image;

        FString Error;

        if (!FFaceSDFImageIO::LoadGrayscalePNG(
            FilePath,
            Image,
            Error))
        {
            SetStatus(
                Error,
                true);

            OutImages.Reset();

            return false;
        }

        OutImages.Add(
            MoveTemp(Image));
    }

    return true;
}

// =============================================================================
// 输出目录
// =============================================================================

FString
SFaceSDFGeneratorWindow::
GetShadowMaskOutputDirectory() const
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("FaceSDF"),
        TEXT("ShadowMasks"));
}

FString
SFaceSDFGeneratorWindow::
GetSDFOutputDirectory() const
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("FaceSDF"),
        TEXT("GrayscaleSDF"));
}

FString
SFaceSDFGeneratorWindow::
GetAtlasOutputDirectory() const
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("FaceSDF"),
        TEXT("Atlas"));
}

// =============================================================================
// 文件命名
// =============================================================================

FString
SFaceSDFGeneratorWindow::MakeSDFFileName(
    const FString& ShadowMaskPath) const
{
    FString BaseName =
        FPaths::GetBaseFilename(
            ShadowMaskPath);

    if (BaseName.StartsWith(
        TEXT("FaceShadow_")))
    {
        BaseName =
            BaseName.RightChop(11);
    }
    else if (BaseName.StartsWith(
        TEXT("Shadow_")))
    {
        BaseName =
            BaseName.RightChop(7);
    }

    return FString::Printf(
        TEXT("SDF_%s.png"),
        *BaseName);
}

FString
SFaceSDFGeneratorWindow::GetSafeOutputName() const
{
    FString SafeName =
        OutputName;

    SafeName.TrimStartAndEndInline();

    SafeName.ReplaceInline(
        TEXT("/"),
        TEXT("_"));

    SafeName.ReplaceInline(
        TEXT("\\"),
        TEXT("_"));

    SafeName.ReplaceInline(
        TEXT(":"),
        TEXT("_"));

    if (SafeName.IsEmpty())
    {
        SafeName =
            TEXT("FaceSDF_Atlas");
    }

    return SafeName;
}

// =============================================================================
// 状态
// =============================================================================

void SFaceSDFGeneratorWindow::SetStatus(
    const FString& Message,
    bool bIsError)
{
    StatusMessage =
        Message;

    bLastStatusWasError =
        bIsError;

    if (bIsError)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Face SDF: %s"),
            *Message);
    }
    else
    {
        UE_LOG(
            LogTemp,
            Display,
            TEXT("Face SDF: %s"),
            *Message);
    }
}

#undef LOCTEXT_NAMESPACE