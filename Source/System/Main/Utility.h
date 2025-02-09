#pragma once

#include <string>
#include <cmath>
#include <algorithm>
#include <Windows.h>
#include <DirectXMath.h>
#include <vector>

#include "..\\Engine\\Core\\Hash\\xxhash.h"

constexpr const UINT HASH_SAMPLERATE = 10;

extern std::string UTF8ToShiftJIS(std::string utf8Str);

//�w�肵����������L��ID�𐶐�
//���ɂ܂�ɕʂ̕����񂩂瓯��ID����������� (��������1)
extern UINT HashString(const std::string& str, bool use32bits = true);

//XMFLOAT3�̐��`���
//���� : ��Ԍ���XMFLOAT3, ��Ԑ��XMFLOAT3, ��Ԓn�_(0.0f�`1.0f)
extern DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t);
//XMFLOAT4�̐��`���
//���� : ��Ԍ���XMFLOAT4, ��Ԑ��XMFLOAT4, ��Ԓn�_(0.0f�`1.0f)
extern DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t);

//�t�@�C���p�X����g���q�𒊏o
extern std::string GetFileExtension(const std::string& filePath);

//string����wstring�ւ̕ϊ�
extern std::wstring GetWideString(const std::string& str);
extern std::string GetNarrowString(const std::wstring& str);

extern std::vector<std::string> GetSprits(std::string& str, char delim);

//�t�@�C�����e�����ӂ�ID�𐶐�
extern UINT GenerateIDFromFile(const std::string& filePath);
extern UINT GenerateIDFromFile(const std::wstring& filePath);
extern UINT GenerateIDFromFile (const std::vector<char>& buffer);

//RGBA����UINT�֕ϊ�
extern UINT ColorToUINT(float r, float g, float b, float a = 1.0f);
