#pragma once

constexpr int FRAME_BUFFER_COUNT = 3;

#ifdef _DEBUG
#define BUILD_MODE L"Resource\\DebugShaders\\"
#else
#define BUILD_MODE L"Resource\\ReleaseShaders\\"
#endif

namespace KeyString
{
	constexpr const wchar_t* SHADER_BONE_VERTEX = BUILD_MODE "BoneVS.cso";
	constexpr const wchar_t* SHADER_TEXTURE_PIXEL = BUILD_MODE "ToonPS.cso";
	constexpr const wchar_t* SHADER_SHAPE_CONVERT = BUILD_MODE "ConvertShapeData.cso";
	constexpr const wchar_t* SHADER_PRIMITIVE_PIXEL = BUILD_MODE "PrimitivePS.cso";
	constexpr const wchar_t* SHADER_PRIMITIVE_VERTEX = BUILD_MODE "PrimitiveVS.cso";
	constexpr const wchar_t* SHADER_TERRAIN_PIXEL = BUILD_MODE "TerrainPS.cso";
	constexpr const wchar_t* SHADER_GRASS_VERTEX = BUILD_MODE "GrassVS.cso";
	constexpr const wchar_t* SHADER_GRASS_PIXEL = BUILD_MODE "GrassPS.cso";
	constexpr const wchar_t* SHADER_SHADOW_VERTEX = BUILD_MODE "ShadowVS.cso";
	constexpr const wchar_t* SHADER_SKYBOX_VERTEX = BUILD_MODE "SkyBoxVS.cso";
	constexpr const wchar_t* SHADER_SKYBOX_PIXEL = BUILD_MODE "SkyBoxPS.cso";
	constexpr const wchar_t* SHADER_UITEXTURE_VERTEX = BUILD_MODE "UITextureVS.cso";
	constexpr const wchar_t* SHADER_UITEXTURE_PIXEL = BUILD_MODE "UITexturePS.cso";
	constexpr const wchar_t* SHADER_POSTPROCESS_VERTEX = BUILD_MODE "PostProcessVS.cso";
	constexpr const wchar_t* SHADER_POSTPROCESS_PIXEL = BUILD_MODE "PostProcessNoisePS.cso";

	constexpr const wchar_t* SHADER_RAYTRACING_RAYGEN = BUILD_MODE "RayGenShader.cso";
	constexpr const wchar_t* SHADER_RAYTRACING_MISS = BUILD_MODE "MissShader.cso";
	constexpr const wchar_t* SHADER_RAYTRACING_HIT = BUILD_MODE "HitShader.cso";
}