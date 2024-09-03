
#include "FBXLoader.h"
#include "..\\SharedStruct\\SharedStruct.h"
#include <assimp\\Importer.hpp>
#include <assimp\\scene.h>
#include <assimp\\postprocess.h>
#include <d3dx12.h>
#include <filesystem>

using namespace DirectX;
namespace fs = std::filesystem;

static std::wstring GetDirectoryPath(const std::wstring& origin)
{
    fs::path p = origin.c_str();
    return p.remove_filename().c_str();
}

// std::string(マルチバイト文字列)からstd::wstring(ワイド文字列)を得る
static std::wstring ToWideString(const std::string& str)
{
    auto num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);

    std::wstring wstr;
    wstr.resize(num1);

    auto num2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, &wstr[0], num1);

    assert(num1 == num2);
    return wstr;
}

bool FBXLoader::Load(ImportSettings settings)
{
    if (settings.filename == nullptr)
    {
        return false;
    }

    std::vector<Mesh>& meshes = settings.meshes;
    bool inverseU = settings.inverseU;
    bool inverseV = settings.inverseV;

    auto path = ToUTF8(settings.filename);

    Assimp::Importer importer;
    int flag = 0;
    flag |= aiProcess_Triangulate;
    flag |= aiProcess_CalcTangentSpace;
    flag |= aiProcess_GenSmoothNormals;
    flag |= aiProcess_GenUVCoords;
    flag |= aiProcess_PopulateArmatureData;

    const aiScene* scene = importer.ReadFile(path, flag);

    if (scene == nullptr)
    {
        // もし読み込みエラーがでたら表示する
        printf(importer.GetErrorString());
        printf("\n");
        return false;
    }

    // 読み込んだデータを自分で定義したMesh構造体に変換する
    meshes.clear();
    meshes.resize(scene->mNumMeshes);
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        aiMesh* pMesh = scene->mMeshes[i];
        LoadMesh(meshes[i], pMesh, inverseU, inverseV);
        aiMaterial* pMaterial = scene->mMaterials[pMesh->mMaterialIndex];
        LoadTexture(settings.filename, meshes[i], pMaterial, pMesh->mMaterialIndex);
    }

    printf("AnimationCount = %d\n", scene->mNumAnimations);
    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        printf("%d -> %s\n", i, scene->mAnimations[i]->mName.C_Str());

    printf("MaterialCount = %d\n", scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        printf("%d -> %s\n", i, scene->mMaterials[i]->GetName().C_Str());
    }
    scene = nullptr;

    return true;
}

void FBXLoader::LoadMesh(Mesh& dst, const aiMesh* src, bool inverseU, bool inverseV)
{
    aiVector3D zero3D(0.0f, 0.0f, 0.0f);
    aiColor4D zeroColor(0.0f, 0.0f, 0.0f, 0.0f);

    dst.Vertices.resize(src->mNumVertices);

    for (auto i = 0u; i < src->mNumVertices; ++i)
    {
        auto position = &(src->mVertices[i]);
        auto normal = &(src->mNormals[i]);
        auto uv = (src->HasTextureCoords(0)) ? &(src->mTextureCoords[0][i]) : &zero3D;
        auto tangent = (src->HasTangentsAndBitangents()) ? &(src->mTangents[i]) : &zero3D;
        auto color = (src->HasVertexColors(0)) ? &(src->mColors[0][i]) : &zeroColor;

        // 反転オプションがあったらUVを反転させる
        if (inverseU)
        {
            uv->x = 1 - uv->x;
        }
        if (inverseV)
        {
            uv->y = 1 - uv->y;
        }

        Vertex vertex = {};
        vertex.Position = XMFLOAT3(position->x, position->y, position->z);
        vertex.Normal = XMFLOAT3(normal->x, normal->y, normal->z);
        vertex.UV = XMFLOAT2(uv->x, uv->y);
        vertex.Tangent = XMFLOAT3(tangent->x, tangent->y, tangent->z);
        vertex.Color = XMFLOAT4(color->r, color->g, color->b, color->a);

        dst.Vertices[i] = vertex;
    }

    dst.Indices.resize(static_cast<std::vector<uint32_t, std::allocator<uint32_t>>::size_type>(src->mNumFaces) * 3);

    for (auto i = 0u; i < src->mNumFaces; ++i)
    {
        const auto& face = src->mFaces[i];

        dst.Indices[static_cast<std::vector<uint32_t, std::allocator<uint32_t>>::size_type>(i) * 3 + 0] = face.mIndices[0];
        dst.Indices[static_cast<std::vector<uint32_t, std::allocator<uint32_t>>::size_type>(i) * 3 + 1] = face.mIndices[1];
        dst.Indices[static_cast<std::vector<uint32_t, std::allocator<uint32_t>>::size_type>(i) * 3 + 2] = face.mIndices[2];
    }
    dst.MeshName = src->mName.C_Str();
}

void FBXLoader::LoadTexture(const wchar_t* filename, Mesh& dst, const aiMaterial* src, const unsigned int materialIndex)
{
    if (src == nullptr) {
        return;
    }

    aiString path;
    auto dir = GetDirectoryPath(filename);
    if (materialIndex == 0) {
        dst.DiffuseMap = dir + ToWideString("Cloth.png");
        return;
    }
    else if (materialIndex == 1) {
        dst.DiffuseMap = dir + ToWideString("Hair.png");
        return;
    }
    else if (materialIndex == 2) {
        dst.DiffuseMap = dir + ToWideString("Body.png");
        return;
    }
    else if (materialIndex == 3) {
        dst.DiffuseMap = dir + ToWideString("Effect.png");
        return;
    }
    else if (materialIndex == 4) {
        dst.DiffuseMap = dir + ToWideString("Face.png");
        return;
    }

    if (src->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
    {
        // テクスチャパスは相対パスで入っているので、ファイルの場所とくっつける
        auto dir = GetDirectoryPath(filename);
        auto file = std::string(path.C_Str());
        dst.DiffuseMap = dir + ToWideString(file);
    }
    else
    {
        dst.DiffuseMap.clear();
    }
}