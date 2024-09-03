#pragma once

#define NOMINMAX
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

struct Mesh;
struct Vertex;

struct aiMesh;
struct aiMaterial;

struct ImportSettings // �C���|�[�g����Ƃ��̃p�����[�^
{
    const wchar_t* filename = nullptr; // �t�@�C���p�X
    std::vector<Mesh>& meshes; // �o�͐�̃��b�V���z��
    bool inverseU = false; // U���W�𔽓]�����邩
    bool inverseV = false; // V���W�𔽓]�����邩
};

class FBXLoader
{
public:
    bool Load(ImportSettings setting); // ���f�������[�h����

    static std::string ToUTF8(const std::wstring& value)
    {
        auto length = WideCharToMultiByte(CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);
        auto buffer = new char[length];

        WideCharToMultiByte(CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

        std::string result(buffer);
        delete[] buffer;
        buffer = nullptr;

        return result;
    }

private:
    void LoadMesh(Mesh& dst, const aiMesh* src, bool inverseU, bool inverseV);
    void LoadTexture(const wchar_t* filename, Mesh& dst, const aiMaterial* src, const unsigned int materialIndex);
};
