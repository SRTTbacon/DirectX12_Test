#pragma once

#include <regex>
#include <Windows.h>
#include <cmath>
#include <DirectXMath.h>

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