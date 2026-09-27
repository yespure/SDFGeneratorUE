#pragma once

#include "CoreMinimal.h"
#include "FaceSDFTypes.h"

class USkeletalMesh;

struct FFaceSDFPipelineRequest
{
	USkeletalMesh* Mesh = nullptr;

	FFaceSDFMeshSettings MeshSettings;
	FFaceSDFGenerationSettings GenerationSettings;
};

class FFaceSDFPipeline
{
public: static FFaceSDFOperationResult
	GenerateShadowMasks(
		const FFaceSDFPipelineRequest& Request,
		TArray<FFaceSDFGrayImage>& OutMasks,
		TArray<FFaceSDFLightSample>& OutSamples);

      static FFaceSDFOperationResult
          ConvertMasksToSDF(
              const TArray<FFaceSDFGrayImage>& Masks,
              TArray<FFaceSDFGrayImage>& OutSDFImages);

      static FFaceSDFOperationResult
          BuildAtlas(
              const TArray<FFaceSDFGrayImage>& SDFImages,
              const TArray<FFaceSDFLightSample>& Samples,
              FFaceSDFGrayImage& OutAtlas);

      static FFaceSDFOperationResult
          GenerateAtlasFromMesh(
              const FFaceSDFPipelineRequest& Request,
              FFaceSDFGrayImage& OutAtlas);
};