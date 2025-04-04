#pragma once

#include <string>
#include <cmath>
#include <algorithm>
#include <Windows.h>
#include <DirectXMath.h>
#include <vector>
#include <cwchar>
#include <d3dx12.h>
#include <DirectXTex.h>

#include "..\\Engine\\Core\\Hash\\xxhash.h"
#include "..\\ComPtr.h"

constexpr const UINT HASH_SAMPLERATE = 10;

extern std::string UTF8ToShiftJIS(std::string utf8Str);

//指定した文字列特有のIDを生成
//超極まれに別の文字列から同じIDが生成される (数億分の1)
extern UINT HashString(const std::string& str, bool use32bits = true);

//XMFLOAT3の線形補間
//引数 : 補間元のXMFLOAT3, 補間先のXMFLOAT3, 補間地点(0.0f～1.0f)
extern DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t);
//XMFLOAT4の線形補間
//引数 : 補間元のXMFLOAT4, 補間先のXMFLOAT4, 補間地点(0.0f～1.0f)
extern DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t);

//ファイルパスから拡張子を抽出 ('.'付き)
extern std::string GetFileExtension(const std::string& filePath);
extern std::wstring GetFileExtension(const std::wstring& filePath);

//stringからwstringへの変換
extern std::wstring GetWideString(const std::string& str);
extern std::u16string GetU16String(const std::string& str);
extern std::string GetNarrowString(const std::wstring& str);
extern std::string GetStringFromU16(const std::u16string& str);

extern std::vector<std::string> GetSprits(std::string& str, char delim);

//ファイル内容から一意のIDを生成
extern UINT GenerateIDFromFile(const std::string& filePath);
extern UINT GenerateIDFromFile(const std::wstring& filePath);
extern UINT GenerateIDFromFile(const std::vector<char>& buffer);
extern UINT GenerateIDFromFile(const char* data, size_t size);

//RGBAからUINTへ変換
extern UINT ColorToUINT(float r, float g, float b, float a = 1.0f);

extern DirectX::XMVECTOR ExtractEulerAngles(const DirectX::XMMATRIX& matrix);
