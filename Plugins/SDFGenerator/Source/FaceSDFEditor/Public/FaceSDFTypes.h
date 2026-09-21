#pragma once

#include "CoreMinimal.h"

struct FFaceSDFTriangle
{
    FVector2D UV0;
    FVector2D UV1;
    FVector2D UV2;

    // 三角形三个顶点的模型局部空间位置
    FVector3f Position0;
    FVector3f Position1;
    FVector3f Position2;

    // 三角形三个顶点的模型局部空间法线
    FVector3f Normal0;
    FVector3f Normal1;
    FVector3f Normal2;
};//三角形信息