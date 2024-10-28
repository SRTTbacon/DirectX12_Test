#pragma once
#include "System\\ComPtr.h"

constexpr int FRAME_BUFFER_COUNT = 3;

namespace KeyString
{
	constexpr const wchar_t* SHADER_BONE_VERTEX = L"x64\\Debug\\BoneVS.cso";
	constexpr const wchar_t* SHADER_TEXTURE_PIXEL = L"x64\\Debug\\TexturePS.cso";
	constexpr const wchar_t* SHADER_SHAPE_CONVERT = L"x64\\Debug\\ConvertShapeData.cso";
	constexpr const wchar_t* SHADER_PRIMITIVE_PIXEL = L"x64\\Debug\\PrimitivePS.cso";
	constexpr const wchar_t* SHADER_PRIMITIVE_VERTEX = L"x64\\Debug\\PrimitiveVS.cso";
	constexpr const wchar_t* SHADER_SHADOW_VERTEX = L"x64\\Debug\\ShadowVS.cso";
}