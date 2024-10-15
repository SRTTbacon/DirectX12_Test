#pragma once

#include <regex>
#include <Windows.h>
#include <cmath>
#include <DirectXMath.h>

extern std::string UTF8ToShiftJIS(std::string utf8Str);

extern UINT HashString(const std::string& str);
extern UINT FnvHash(const std::vector<uint8_t>& input, bool use32bits);

extern DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t);
extern DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t);