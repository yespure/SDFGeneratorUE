# SDF Generator

An SDF texture generation tool for stylized character face shading in Unreal Engine.

SDF Generator converts black-and-white shadow masks into grayscale Signed Distance Fields, providing distance data for face shadow materials. Built with C++ and Slate, the project also explores generating lighting masks directly from Skeletal Mesh face geometry.

> **Status: In development.** Batch PNG conversion, SDF computation, and atlas packing have code implementations. Mesh lighting, nose occlusion, and the complete automated baking workflow are still being developed and validated.

## Features

- **Batch PNG import:** Select multiple shadow masks and process them in sorted file-path order.
- **Grayscale SDF generation:** Convert binary masks into 8-bit grayscale distance fields.
- **Texture asset output:** Save generated results as Unreal Engine texture assets.
- **Face geometry extraction:** Read triangles, positions, and normals from a selected Skeletal Mesh LOD, Section, and UV Channel.
- **Atlas packing prototype:** Arrange 65 SDF PNGs into a 9×9 texture atlas.
- **Experimental lighting baking:** Develop shadow masks based on face normals and geometric occlusion.

## Workflow

```text
Shadow Mask PNGs → Grayscale conversion and thresholding → SDF computation → UE texture assets

65 SDF PNGs → 9×9 atlas packing → Atlas texture asset
```

The mesh baking workflow is still in development:

```text
Skeletal Mesh → Face triangle extraction → UV rasterization
              → Directional lighting and geometric occlusion → Shadow Mask → SDF
```

A face UV coverage mask describes the region occupied by the face in UV space. A lighting shadow mask describes the light and shadow distribution for a specific light direction. SDFs intended for face shadow shading should use the corresponding lighting masks as input.

## Installation and Build

This project is intended for use as an Unreal Engine C++ editor plugin. Refer to the plugin configuration in the repository for engine version and toolchain requirements.

1. Place the complete plugin directory, including its `.uplugin` file, in your project's `Plugins/` folder.
2. Close Unreal Editor and regenerate the project files.
3. For the Windows / Visual Studio workflow, select `Development Editor` and `Win64`, then build the project.
4. Reopen the project, enable the plugin in the Plugins window, and restart the editor if prompted.
5. Open the Face SDF Generator window registered by the plugin.

## Usage

### Batch SDF Generation

1. Prepare square PNG shadow masks with matching dimensions.
2. Click **Select Shadow Mask PNGs** and select the files to convert.
3. Click **Generate All SDF**.
4. Inspect the generated textures in the Content Browser and check the Output Log for processing results.

Use zero-padded filenames to preserve the order of your directional sequence:

```text
Shadow_00.png
Shadow_01.png
...
Shadow_64.png
```

`Shadow_00.png` produces an asset named `SDF_00`. Other filenames receive an `SDF_` prefix. Batch conversion does not require exactly 65 images; that requirement applies to the current atlas prototype.

Input images are converted to grayscale. Pixel values greater than `127` are treated as inside the mask; all other values are treated as outside. Use grayscale intensity to represent the mask.

### Mesh Parameters

| Parameter | Description |
| --- | --- |
| Select Mesh | Skeletal Mesh containing the face geometry |
| LOD | LOD index used for triangle extraction |
| Section | Section index containing the target face geometry |
| UV Channel | UV channel index used for rasterization |
| Resolution | Width and height of the mesh rasterization output |
| Output Name | Output asset name for a single SDF |

Batch PNG conversion determines the resolution from the input image dimensions. For mesh lighting tests, start at `128×128` and verify the face region, normals, and light direction before increasing the resolution.

### Atlas Prototype

The current atlas code reads **65 existing SDF PNGs** and packs them row by row into a **9×9** grid. The output asset is named `FaceSDF_Atlas` and is saved under `/Game/FaceSDF/`.

- All inputs should be square images with matching dimensions.
- For an input tile size of `N×N`, the atlas size is `9N×9N`.
- The atlas uses 65 of its 81 cells. The remaining 16 cells do not represent valid directions.
- The atlas uses a grayscale texture with sRGB and mipmaps disabled, and bilinear filtering enabled.

The connection between the atlas callback and the editor button still needs to be completed. Batch SDF output and atlas PNG input are currently separate workflows and do not yet form a one-click generation pipeline.

## SDF Encoding

The current implementation uses two-pass distance propagation over an eight-connected neighborhood. Horizontal and vertical steps cost `1`, while diagonal steps cost `√2`, producing an approximate distance field.

| Region | Distance sign | Grayscale meaning |
| --- | --- | --- |
| Inside the mask | Positive | Above 128; brighter farther inside |
| Near the boundary | Near zero | Near 128 |
| Outside the mask | Negative | Below 128; darker farther outside |

Encoding formula:

```text
value = round(128 + clamp(signedDistance / 64, -1, 1) × 127)
```

Distances are measured in pixels, with a fixed normalization range of 64 pixels. Changing the input resolution changes this range relative to the overall texture size. Materials should sample the texture as linear data and set thresholds according to this encoding convention.

## Limitations and Future Work

- The tool currently targets offline texture generation within the editor.
- Mesh input uses imported Skeletal Mesh geometry; an animated-pose baking workflow is not yet available.
- Extracting a single Section does not automatically include noses, hair, or other occluders in separate Sections.
- Overlapping UVs, an incorrect Section, or mismatched lighting coordinate conventions may affect baking results.
- Geometric occlusion remains experimental. Performance at high resolutions and triangle counts requires further optimization.
- Atlas direction mapping, material sampling, and tile-edge bleeding need to be handled as part of the material workflow.

Future work includes validating single-direction lighting, automating baking across multiple directions, and connecting shadow mask generation, SDF conversion, and atlas packing into a complete workflow.

## Reporting Issues

When opening an issue, include your Unreal Engine version, reproduction steps, input image dimensions or mesh parameters, and relevant Output Log entries containing `Face SDF:`.
