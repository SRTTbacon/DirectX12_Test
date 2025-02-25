#pragma once
#include "System\\ComPtr.h"

constexpr int FRAME_BUFFER_COUNT = 3;

namespace KeyString
{
	constexpr const wchar_t* SHADER_BONE_VERTEX = L"x64\\Debug\\BoneVS.cso";
	constexpr const wchar_t* SHADER_TEXTURE_PIXEL = L"x64\\Debug\\ToonPS.cso";
	constexpr const wchar_t* SHADER_SHAPE_CONVERT = L"x64\\Debug\\ConvertShapeData.cso";
	constexpr const wchar_t* SHADER_PRIMITIVE_PIXEL = L"x64\\Debug\\PrimitivePS.cso";
	constexpr const wchar_t* SHADER_PRIMITIVE_VERTEX = L"x64\\Debug\\PrimitiveVS.cso";
	constexpr const wchar_t* SHADER_SHADOW_VERTEX = L"x64\\Debug\\ShadowVS.cso";

	constexpr const wchar_t* SHADER_RAYTRACING_RAYGEN = L"x64\\Debug\\RayGenShader.cso";
	constexpr const wchar_t* SHADER_RAYTRACING_MISS = L"x64\\Debug\\MissShader.cso";
	constexpr const wchar_t* SHADER_RAYTRACING_HIT = L"x64\\Debug\\HitShader.cso";
}