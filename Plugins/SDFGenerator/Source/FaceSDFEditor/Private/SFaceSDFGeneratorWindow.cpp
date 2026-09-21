#include "SFaceSDFGeneratorWindow.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "PropertyCustomizationHelpers.h"

#include "Engine/SkeletalMesh.h"

#include "FaceSDFGenerator.h"
#include "FaceSDFTexture.h"

#include "Framework/Application/SlateApplication.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "SFaceSDFGeneratorWindow"

static FVector3f MakeFaceSDFLightDirection(
    float YawDegrees,
    float PitchDegrees)
{
    const float YawRadians =
        FMath::DegreesToRadians(
            YawDegrees);

    const float PitchRadians =
        FMath::DegreesToRadians(
            PitchDegrees);

    const float CosPitch =
        FMath::Cos(PitchRadians);

    // 模型脸部正前方为+Y
    const FVector3f Forward(
        0.0f,
        1.0f,
        0.0f);

    // 模型脸部右侧为-X
    const FVector3f Right(
        -1.0f,
        0.0f,
        0.0f);

    // 模型头顶方向为+Z
    const FVector3f Up(
        0.0f,
        0.0f,
        1.0f);

    const FVector3f Direction =
        Forward *
        (
            CosPitch *
            FMath::Cos(YawRadians)
            )
        +
        Right *
        (
            CosPitch *
            FMath::Sin(YawRadians)
            )
        +
        Up *
        FMath::Sin(PitchRadians);

    return Direction.GetSafeNormal();
}

void SFaceSDFGeneratorWindow::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SVerticalBox)

                // 标题
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.0f)
                [
                    SNew(STextBlock)
                        .Text(LOCTEXT("WindowTitle", "Face SDF Generator V1"))
                ]

                //Mesh部分
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock)
                                .Text(LOCTEXT("SelectMeshLabel", "Select Mesh:"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SObjectPropertyEntryBox)
                                .AllowedClass(USkeletalMesh::StaticClass())
                                // skeletal mesh only
                                .ObjectPath(this, &SFaceSDFGeneratorWindow::GetSelectedMeshAsset)
                                // 获取当前选中的资源
                                .OnObjectChanged(this, &SFaceSDFGeneratorWindow::OnMeshChanged)
                                // 改变时的回调
                        ]
                ]

            //LOD部分
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            // ✅修复：LOCTEXT
                            SNew(STextBlock).Text(LOCTEXT("LODLabel", "LOD:"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(this, &SFaceSDFGeneratorWindow::GetLODValue)
                                .OnValueChanged(this, &SFaceSDFGeneratorWindow::OnLODChanged)
                                .MinValue(0)
                        ]
                ]

            //Section部分
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock).Text(LOCTEXT("SectionLabel", "Section:"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(this, &SFaceSDFGeneratorWindow::GetSectionValue)
                                .OnValueChanged(this, &SFaceSDFGeneratorWindow::OnSectionChanged)
                                .MinValue(0)
                        ]
                ]

            //UVChannel部分
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock).Text(LOCTEXT("UVChannelLabel", "UV Channel:"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(this, &SFaceSDFGeneratorWindow::GetUVChannelValue)
                                .OnValueChanged(this, &SFaceSDFGeneratorWindow::OnUVChannelChanged)
                                .MinValue(0)
                        ]
                ]

            //Resolution部分
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock).Text(LOCTEXT("ResolutionLabel", "Resolution:"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SNumericEntryBox<int32>)
                                .Value(this, &SFaceSDFGeneratorWindow::GetResolutionValue)
                                .OnValueChanged(this, &SFaceSDFGeneratorWindow::OnResolutionChanged)
                                .MinValue(64)
                        ]
                ]

            //OutputName部分
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(STextBlock).Text(LOCTEXT("OutputNameLabel", "Output Name:"))
                        ]

                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SEditableTextBox)
                                .Text(this, &SFaceSDFGeneratorWindow::GetOutputNameText)
                                .OnTextChanged(this, &SFaceSDFGeneratorWindow::OnOutputNameChanged)
                        ]
                ]

            // ===== Shadow Mask 批量导入 =====
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(0, 0, 10, 0)
                        [
                            SNew(SButton)
                                .Text(LOCTEXT("SelectShadowMasksBtn", "Select Shadow Mask PNGs"))
                                .OnClicked(
                                    FOnClicked::CreateSP(
                                        this,
                                        &SFaceSDFGeneratorWindow::OnSelectShadowMasksClicked))
                        ]

                    + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Text(
                                    this,
                                    &SFaceSDFGeneratorWindow::GetShadowMaskStatusText)
                        ]
                ]

            // ===== 批量生成SDF =====
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(8.0f)
                [
                    SNew(SButton)
                        .Text(LOCTEXT("GenerateAllSDFBtn", "Generate All SDF"))
                        .OnClicked(
                            FOnClicked::CreateSP(
                                this,
                                &SFaceSDFGeneratorWindow::OnGenerateAllSDFClicked))
                ]

            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(20.0f)
                .HAlign(HAlign_Center)
                [
                    SNew(SButton)
                        .Text(LOCTEXT("GenerateBtn", "Generate SDF"))
                        .OnClicked(
                            FOnClicked::CreateSP(
                                this,
                                &SFaceSDFGeneratorWindow::OnGenerateClicked))
                ]
            //生成Atlas
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(20.0f)
                .HAlign(HAlign_Center)
                [
                    SNew(SButton)
                        .Text(LOCTEXT(
                            "GenerateAtlasBtn",
                            "Generate SDF Atlas From Mesh"))
                        .OnClicked(
                            FOnClicked::CreateSP(
                                this,
                                &SFaceSDFGeneratorWindow::OnGenerateAtlasClicked))
                ]
        ];
}


FReply SFaceSDFGeneratorWindow::OnGenerateClicked()
{
    // 获取选中模型的面部三角形
    TArray<FFaceSDFTriangle> FaceTriangles;

    if (!FFaceSDFGenerator::ExtractFaceTriangles(
        SelectedMesh.Get(),
        LODIndex,
        SectionIndex,
        UVChannel,
        FaceTriangles))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Face SDF: Failed to extract face triangles."));

        return FReply::Handled();
    }

    TArray<FVector3f> LightDirections;
    TArray<FString> OutputNames;

    // 第一张：正上方光源
    LightDirections.Add(
        FVector3f(0.0f, 0.0f, 1.0f));

    OutputNames.Add(
        TEXT("FaceShadow_01_01"));

    // 中间7行的俯仰角
    const float PitchAngles[] =
    {
        67.5f,
        45.0f,
        22.5f,
        0.0f,
        -22.5f,
        -45.0f,
        -67.5f
    };

    // 每行从左到右的9个水平角
    const float YawAngles[] =
    {
        -90.0f,
        -67.5f,
        -45.0f,
        -22.5f,
        0.0f,
        22.5f,
        45.0f,
        67.5f,
        90.0f
    };

    // 生成中间7行，每行9个方向
    for (int32 PitchIndex = 0;
        PitchIndex < 7;
        ++PitchIndex)
    {
        // Atlas中的实际行号为2～8
        const int32 Row =
            PitchIndex + 2;

        for (int32 YawIndex = 0;
            YawIndex < 9;
            ++YawIndex)
        {
            // Atlas中的实际列号为1～9
            const int32 Column =
                YawIndex + 1;

            LightDirections.Add(
                MakeFaceSDFLightDirection(
                    YawAngles[YawIndex],
                    PitchAngles[PitchIndex]));

            OutputNames.Add(
                FString::Printf(
                    TEXT("FaceShadow_%02d_%02d"),
                    Row,
                    Column));
        }
    }

    // 最后一张：正下方光源
    LightDirections.Add(
        FVector3f(0.0f, 0.0f, -1.0f));

    OutputNames.Add(
        TEXT("FaceShadow_09_01"));

    if (LightDirections.Num() != 65 ||
        OutputNames.Num() != 65)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Face SDF: Invalid light direction count."));

        return FReply::Handled();
    }

    int32 GeneratedCount = 0;

    // 根据所有方向依次生成Shadow Mask
    for (int32 LightIndex = 0;
        LightIndex < LightDirections.Num();
        ++LightIndex)
    {
        TArray<uint8> ShadowMaskPixels;

        if (!FFaceSDFGenerator::RasterizeShadowMask(
            FaceTriangles,
            LightDirections[LightIndex],
            Resolution,
            ShadowMaskPixels))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("Face SDF: Failed to generate Shadow Mask %d."),
                LightIndex);

            continue;
        }

        if (!FFaceSDFTexture::SaveFaceMaskTexture(
            ShadowMaskPixels,
            Resolution,
            OutputNames[LightIndex]))
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("Face SDF: Failed to save %s."),
                *OutputNames[LightIndex]);

            continue;
        }

        ++GeneratedCount;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Generated %s, Direction=(%f, %f, %f)"),
            *OutputNames[LightIndex],
            LightDirections[LightIndex].X,
            LightDirections[LightIndex].Y,
            LightDirections[LightIndex].Z);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Face SDF: Lighting generation completed. %d / %d generated."),
        GeneratedCount,
        LightDirections.Num());

    return FReply::Handled();
}

FReply SFaceSDFGeneratorWindow::OnGenerateAtlasClicked()
{
    if (!SelectedMesh.IsValid())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Please select a Skeletal Mesh."));

        return FReply::Handled();
    }

    if (Resolution <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Invalid resolution."));

        return FReply::Handled();
    }

    // 只需要从模型提取一次三角形
    TArray<FFaceSDFTriangle> FaceTriangles;

    if (!FFaceSDFGenerator::ExtractFaceTriangles(
        SelectedMesh.Get(),
        LODIndex,
        SectionIndex,
        UVChannel,
        FaceTriangles))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Face SDF: Failed to extract face triangles."));

        return FReply::Handled();
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Face SDF: Starting direct Atlas generation. "
            "Triangles=%d Resolution=%d."),
        FaceTriangles.Num(),
        Resolution);

    TArray<uint8> AtlasPixels;
    int32 AtlasResolution = 0;

    // 直接通过模型生成65个方向的SDF Atlas
    if (!GenerateSDFAtlas(
        FaceTriangles,
        Resolution,
        AtlasPixels,
        AtlasResolution))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Face SDF: Failed to generate SDF Atlas."));

        return FReply::Handled();
    }

    FString AtlasAssetName = OutputName;

    if (AtlasAssetName.IsEmpty())
    {
        AtlasAssetName =
            TEXT("FaceSDF_Atlas");
    }

    if (!SaveSDFAtlasTexture(
        AtlasPixels,
        AtlasResolution,
        AtlasAssetName))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Face SDF: Failed to save SDF Atlas."));

        return FReply::Handled();
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Face SDF: Atlas generation completed successfully. "
            "Asset=%s Resolution=%d."),
        *AtlasAssetName,
        AtlasResolution);

    return FReply::Handled();
}


void SFaceSDFGeneratorWindow::OnMeshChanged(const FAssetData& AssetData)
{
    // 当用户选择了一个 Mesh 时触发
    SelectedMesh = Cast<USkeletalMesh>(AssetData.GetAsset());

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Mesh changed to: %s"),
        SelectedMesh.IsValid()
        ? *SelectedMesh->GetName()
        : TEXT("None"));
}


FString SFaceSDFGeneratorWindow::GetSelectedMeshAsset() const
{
    if (SelectedMesh.IsValid())
    {
        // 返回资源的路径字符串，UI 就会显示它
        return SelectedMesh->GetPathName();
    }

    return FString(); // 空字符串，UI 就会显示 None
}


//各种数值的获取和设置

TOptional<int32> SFaceSDFGeneratorWindow::GetLODValue() const
{
    return LODIndex;
}


void SFaceSDFGeneratorWindow::OnLODChanged(int32 NewValue)
{
    LODIndex = NewValue;
}


TOptional<int32> SFaceSDFGeneratorWindow::GetSectionValue() const
{
    return SectionIndex;
}


void SFaceSDFGeneratorWindow::OnSectionChanged(int32 NewValue)
{
    SectionIndex = NewValue;
}


TOptional<int32> SFaceSDFGeneratorWindow::GetUVChannelValue() const
{
    return UVChannel;
}


void SFaceSDFGeneratorWindow::OnUVChannelChanged(int32 NewValue)
{
    UVChannel = NewValue;
}


TOptional<int32> SFaceSDFGeneratorWindow::GetResolutionValue() const
{
    return Resolution;
}


void SFaceSDFGeneratorWindow::OnResolutionChanged(int32 NewValue)
{
    Resolution = NewValue;
}


// OutputName 文本双向绑定
FText SFaceSDFGeneratorWindow::GetOutputNameText() const
{
    return FText::FromString(OutputName);
}


void SFaceSDFGeneratorWindow::OnOutputNameChanged(const FText& NewText)
{
    OutputName = NewText.ToString();
}


// ===== Shadow Mask 批量导入 =====

FReply SFaceSDFGeneratorWindow::OnSelectShadowMasksClicked()
{
    IDesktopPlatform* DesktopPlatform =
        FDesktopPlatformModule::Get();

    if (!DesktopPlatform)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Face SDF: Failed to get Desktop Platform."));

        return FReply::Handled();
    }

   const void* ParentWindowHandle =
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

    TArray<FString> SelectedFiles;

    const bool bOpened =
        DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            TEXT("Select Shadow Mask PNGs"),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("PNG Files (*.png)|*.png"),
            EFileDialogFlags::Multiple,
            SelectedFiles);

    if (!bOpened || SelectedFiles.Num() == 0)
    {
        return FReply::Handled();
    }

    // 按文件名排序，保证 Shadow_00 ~ Shadow_64 顺序稳定
    SelectedFiles.Sort();

    SelectedShadowMaskFiles = MoveTemp(SelectedFiles);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Face SDF: Selected %d Shadow Mask files."),
        SelectedShadowMaskFiles.Num());

    return FReply::Handled();
}

FText SFaceSDFGeneratorWindow::GetShadowMaskStatusText() const
{
    return FText::Format(
        LOCTEXT(
            "ShadowMaskStatus",
            "{0} Shadow Mask files selected"),
        SelectedShadowMaskFiles.Num());
}


bool SFaceSDFGeneratorWindow::ReadShadowMaskPNG(
    const FString& FilePath,
    TArray<uint8>& OutPixels,
    int32& OutWidth,
    int32& OutHeight)
{
    TArray<uint8> CompressedData;

    if (!FFileHelper::LoadFileToArray(
        CompressedData,
        *FilePath))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to read PNG file: %s"),
            *FilePath);

        return false;
    }

    IImageWrapperModule& ImageWrapperModule =
        FModuleManager::LoadModuleChecked<IImageWrapperModule>(
            TEXT("ImageWrapper"));

    TSharedPtr<IImageWrapper> ImageWrapper =
        ImageWrapperModule.CreateImageWrapper(
            EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to create PNG image wrapper."));

        return false;
    }

    if (!ImageWrapper->SetCompressed(
        CompressedData.GetData(),
        CompressedData.Num()))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to decompress PNG: %s"),
            *FilePath);

        return false;
    }

    OutWidth = ImageWrapper->GetWidth();
    OutHeight = ImageWrapper->GetHeight();

    TArray64<uint8> RawData;

    if (!ImageWrapper->GetRaw(
        ERGBFormat::Gray,
        8,
        RawData))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to convert PNG to grayscale: %s"),
            *FilePath);

        return false;
    }

    if (RawData.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: PNG raw data is invalid: %s"),
            *FilePath);

        return false;
    }

    OutPixels.Reset();

    OutPixels.Append(
        RawData.GetData(),
        RawData.Num());

    return true;
}


FReply SFaceSDFGeneratorWindow::OnGenerateAllSDFClicked()
{
    if (SelectedShadowMaskFiles.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: No Shadow Mask files selected."));

        return FReply::Handled();
    }

    int32 GeneratedCount = 0;

    for (int32 FileIndex = 0;
        FileIndex < SelectedShadowMaskFiles.Num();
        ++FileIndex)
    {
        const FString& FilePath =
            SelectedShadowMaskFiles[FileIndex];

        TArray<uint8> MaskPixels;

        int32 Width = 0;
        int32 Height = 0;

        if (!ReadShadowMaskPNG(
            FilePath,
            MaskPixels,
            Width,
            Height))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Face SDF: Failed to read Shadow Mask: %s"),
                *FilePath);

            continue;
        }

        if (Width != Height)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Face SDF: Shadow Mask is not square: %s"),
                *FilePath);

            continue;
        }

        // 使用第一张图片决定SDF分辨率
        if (FileIndex == 0)
        {
            Resolution = Width;
        }

        if (Width != Resolution ||
            Height != Resolution)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Face SDF: Resolution mismatch: %s"),
                *FilePath);

            continue;
        }

        TArray<uint8> SDFPixels;

        if (!FFaceSDFGenerator::GenerateGrayscaleSDF(
            MaskPixels,
            Resolution,
            SDFPixels))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Face SDF: Failed to generate SDF: %s"),
                *FilePath);

            continue;
        }

        FString BaseName =
            FPaths::GetBaseFilename(FilePath);

        FString SDFAssetName;

        if (BaseName.StartsWith(TEXT("Shadow_")))
        {
            const FString IndexString =
                BaseName.RightChop(7);

            SDFAssetName =
                FString::Printf(
                    TEXT("SDF_%s"),
                    *IndexString);
        }
        else
        {
            SDFAssetName =
                FString::Printf(
                    TEXT("SDF_%s"),
                    *BaseName);
        }

        if (!FFaceSDFTexture::SaveFaceSDFTexture(
            SDFPixels,
            Resolution,
            SDFAssetName))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Face SDF: Failed to save SDF: %s"),
                *FilePath);

            continue;
        }

        ++GeneratedCount;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Generated %s"),
            *SDFAssetName);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Face SDF: Batch generation completed. %d / %d files generated."),
        GeneratedCount,
        SelectedShadowMaskFiles.Num());

    return FReply::Handled();
}

//生成PNG读取
bool SFaceSDFGeneratorWindow::LoadPNGAsGrayscale(
    const FString& FilePath,
    TArray<uint8>& OutPixels,
    int32& OutWidth,
    int32& OutHeight)
{
    TArray<uint8> CompressedData;

    if(!FFileHelper::LoadFileToArray(CompressedData, * FilePath))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to load PNG: %s"),
            *FilePath);

        return false;
    }

    IImageWrapperModule& ImageWrapperModule =
        FModuleManager::LoadModuleChecked<IImageWrapperModule>(
            TEXT("ImagerWrapper")
        );
    TSharedPtr<IImageWrapper> ImageWrapper =
        ImageWrapperModule.CreateImageWrapper(
            EImageFormat::PNG);
    if (!ImageWrapper.IsValid())
    {
        return false;
    }

    if (!ImageWrapper->SetCompressed(
        CompressedData.GetData(),
        CompressedData.Num()))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to decode PNG: %s"),
            *FilePath);

        return false;
    }

    OutWidth = ImageWrapper->GetWidth();
    OutHeight = ImageWrapper->GetHeight();

    TArray64<uint8> RawData;

    if (!ImageWrapper->GetRaw(
        ERGBFormat::Gray,
        8,
        RawData))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to convert PNG to grayscale: %s"),
            *FilePath);

        return false;
    }

    if (RawData.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: PNG raw data is invalid: %s"),
            *FilePath);

        return false;
    }

    OutPixels.Reset();

    OutPixels.Append(
        RawData.GetData(),
        RawData.Num());

    return true;

}

bool SFaceSDFGeneratorWindow::GenerateSDFAtlas(
    const TArray<FFaceSDFTriangle>& FaceTriangles,
    int32 SDFResolution,
    TArray<uint8>& OutAtlasPixels,
    int32& OutAtlasResolution)
{
    if (FaceTriangles.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: No face triangles for Atlas generation."));

        return false;
    }

    if (SDFResolution <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Invalid SDF resolution."));

        return false;
    }

    const int32 AtlasGridSize = 9;

    OutAtlasResolution =
        SDFResolution * AtlasGridSize;

    const int32 AtlasPixelCount =
        OutAtlasResolution *
        OutAtlasResolution;

    // SDF中128表示距离边界为0，因此空白区域初始化为128
    OutAtlasPixels.Init(
        128,
        AtlasPixelCount);

    // 将一张SDF复制到Atlas指定格子
    auto CopySDFToAtlas =
        [&](
            const TArray<uint8>& SDFPixels,
            int32 TargetRow,
            int32 TargetColumn) -> bool
        {
            if (SDFPixels.Num() !=
                SDFResolution * SDFResolution)
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("Face SDF: Invalid SDF pixel count."));

                return false;
            }

            if (TargetRow < 0 ||
                TargetRow >= AtlasGridSize ||
                TargetColumn < 0 ||
                TargetColumn >= AtlasGridSize)
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("Face SDF: Invalid Atlas position Row=%d Column=%d."),
                    TargetRow,
                    TargetColumn);

                return false;
            }

            for (int32 Y = 0;
                Y < SDFResolution;
                ++Y)
            {
                for (int32 X = 0;
                    X < SDFResolution;
                    ++X)
                {
                    const int32 SourceIndex =
                        Y * SDFResolution + X;

                    const int32 AtlasX =
                        TargetColumn * SDFResolution + X;

                    const int32 AtlasY =
                        TargetRow * SDFResolution + Y;

                    const int32 AtlasIndex =
                        AtlasY * OutAtlasResolution +
                        AtlasX;

                    OutAtlasPixels[AtlasIndex] =
                        SDFPixels[SourceIndex];
                }
            }

            return true;
        };

    // 生成单个方向的Shadow Mask和SDF
    auto GenerateOneSDF =
        [&](
            const FVector3f& LightDirection,
            TArray<uint8>& OutSDFPixels) -> bool
        {
            TArray<uint8> ShadowMaskPixels;

            if (!FFaceSDFGenerator::RasterizeShadowMask(
                FaceTriangles,
                LightDirection,
                SDFResolution,
                ShadowMaskPixels))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("Face SDF: Failed to generate Shadow Mask."));

                return false;
            }

            if (!FFaceSDFGenerator::GenerateGrayscaleSDF(
                ShadowMaskPixels,
                SDFResolution,
                OutSDFPixels))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("Face SDF: Failed to generate grayscale SDF."));

                return false;
            }

            return true;
        };

    int32 GeneratedDirectionCount = 0;

    // 第一行：正上方光源
    {
        TArray<uint8> TopSDFPixels;

        const FVector3f TopLightDirection(
            0.0f,
            0.0f,
            -1.0f);

        if (!GenerateOneSDF(
            TopLightDirection,
            TopSDFPixels))
        {
            return false;
        }

        // 极点水平方向没有区别，因此复制到第一行全部9格
        for (int32 Column = 0;
            Column < AtlasGridSize;
            ++Column)
        {
            if (!CopySDFToAtlas(
                TopSDFPixels,
                0,
                Column))
            {
                return false;
            }
        }

        ++GeneratedDirectionCount;
    }

    // 中间7行的俯仰角
    const float PitchAngles[] =
    {
        -67.5f,
        -45.0f,
        -22.5f,
        0.0f,
        22.5f,
        45.0f,
        67.5f
    };

    // 每行从左到右的9个水平角
    // 水平方向完整旋转360度
// 360 / 9 = 40度
    const float YawAngles[] =
    {
        0.0f,
    22.5f,
    45.0f,
    67.5f,
    90.0f,
    112.5f,
    135.0f,
    157.5f,
    180.0f
    };

    // 第二行到第八行，每行生成9个方向
    for (int32 PitchIndex = 0;
        PitchIndex < 7;
        ++PitchIndex)
    {
        const int32 AtlasRow =
            PitchIndex + 1;

        for (int32 YawIndex = 0;
            YawIndex < 9;
            ++YawIndex)
        {
            const int32 AtlasColumn =
                YawIndex;

            const FVector3f LightDirection =
                MakeFaceSDFLightDirection(
                    YawAngles[YawIndex],
                    PitchAngles[PitchIndex]);

            TArray<uint8> SDFPixels;

            if (!GenerateOneSDF(
                LightDirection,
                SDFPixels))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT(
                        "Face SDF: Failed at Row=%d Column=%d."),
                    AtlasRow,
                    AtlasColumn);

                return false;
            }

            if (!CopySDFToAtlas(
                SDFPixels,
                AtlasRow,
                AtlasColumn))
            {
                return false;
            }

            ++GeneratedDirectionCount;

            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "Face SDF: Generated direction %d / 65, Row=%d Column=%d."),
                GeneratedDirectionCount,
                AtlasRow,
                AtlasColumn);
        }
    }

    // 最后一行：正下方光源
    {
        TArray<uint8> BottomSDFPixels;

        const FVector3f BottomLightDirection(
            0.0f,
            0.0f,
            1.0f);

        if (!GenerateOneSDF(
            BottomLightDirection,
            BottomSDFPixels))
        {
            return false;
        }

        // 极点水平方向没有区别，因此复制到最后一行全部9格
        for (int32 Column = 0;
            Column < AtlasGridSize;
            ++Column)
        {
            if (!CopySDFToAtlas(
                BottomSDFPixels,
                8,
                Column))
            {
                return false;
            }
        }

        ++GeneratedDirectionCount;
    }

    if (GeneratedDirectionCount != 65)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Face SDF: Invalid generated direction count: %d."),
            GeneratedDirectionCount);

        return false;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Face SDF: Atlas generated directly from mesh. "
            "Directions=%d, AtlasResolution=%d."),
        GeneratedDirectionCount,
        OutAtlasResolution);

    return true;
}

//保存Atlas
bool SFaceSDFGeneratorWindow::SaveSDFAtlasTexture(
    const TArray<uint8>& Pixels,
    int32 AtlasResolution,
    const FString& AssetName)
{
    if (Pixels.Num() !=
        AtlasResolution * AtlasResolution)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Invalid Atlas pixel count."));

        return false;
    }
    //确定路径以及名称
    FString SafeAssetName =
        ObjectTools::SanitizeObjectName(
            AssetName);

    const FString PackagePath =
        TEXT("/Game/FaceSDF/") +
        SafeAssetName;
    //创建资源包
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return false;
    }

    UTexture2D* Texture = NewObject<UTexture2D>(
        Package,
        *SafeAssetName,
        RF_Public | RF_Standalone);
    if (!Texture)
    {
        return false;
    }

    // 初始化纹理源数据
    Texture->Source.Init(
        AtlasResolution,
        AtlasResolution,
        1,
        1,
        TSF_G8);

    uint8* MipData =
        Texture->Source.LockMip(0);

    FMemory::Memcpy(
        MipData,
        Pixels.GetData(),
        Pixels.Num());

    // 解锁纹理数据
    Texture->Source.UnlockMip(0);

    // 纹理设置
    Texture->SRGB = false;
    Texture->CompressionSettings =
        TC_Grayscale;
    Texture->MipGenSettings =
        TMGS_NoMipmaps;
    Texture->Filter =
        TF_Bilinear;

    Texture->UpdateResource();

    FAssetRegistryModule::AssetCreated(
        Texture);

    Package->MarkPackageDirty();

    const FString PackageFileName =
        FPackageName::LongPackageNameToFilename(
            PackagePath,
            FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;

    SaveArgs.TopLevelFlags =
        RF_Public | RF_Standalone;

    if (!UPackage::SavePackage(
        Package,
        Texture,
        *PackageFileName,
        SaveArgs))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Face SDF: Failed to save SDF Atlas."));

        return false;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Face SDF: SDF Atlas saved: %s"),
        *PackagePath);

    return true;

}

#undef LOCTEXT_NAMESPACE