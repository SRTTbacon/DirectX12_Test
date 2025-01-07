#include "ConvertFromFBX.h"

ConvertResult ConvertFromFBX::ConvertFromCharacter(Character* pCharacter, std::string fromFilePath, std::string toFilePath)
{
    if (!std::filesystem::exists(fromFilePath)) {
        return ConvertResult::FileNotExist;
    }

    if (!bw.OpenWrite(toFilePath)) {
        return ConvertResult::CantWriteFile;
    }

    if (!pCharacter) {
        return ConvertResult::Nullptr;
    }

    Assimp::Importer importer;

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、ボーンが存在する場合は影響を受けるウェイトを4つまでにする
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
    const aiScene* scene = importer.ReadFile(fromFilePath, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return ConvertResult::FbxFormatExeption;
    }

    //ヘッダー
    bw.Write((BYTE)std::strlen(MODEL_HEADER));
    bw.Write(MODEL_HEADER, std::strlen(MODEL_HEADER));
    bw.Write(MODEL_CHARACTER);

    //ボーンとシェイプ情報を保存
    ProcessBone(pCharacter);
    ProcessHumanMesh(pCharacter);

    //メッシュ情報を変換
    bw.Write(scene->mNumMeshes);
    for (UINT i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        ProcessMesh(mesh);
    }

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        //メッシュのマテリアルを取得する
        bw.Write(mesh->mMaterialIndex >= 0);
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //ファイル名 = マテリアル名 + .png
            std::string nameOnly = material->GetName().C_Str() + std::string(".png");
            bw.Write(nameOnly);
        }
    }

    bw.Close();

    m_boneMapping.clear();

    return ConvertResult::Success;
}

void ConvertFromFBX::ProcessMesh(aiMesh* pMesh)
{
    BinaryWriter meshWriter;
    meshWriter.OpenWrite();

    //頂点の処理
    meshWriter.Write(pMesh->mNumVertices);
    for (UINT i = 0; i < pMesh->mNumVertices; i++) {
        meshWriter.Write(pMesh->mVertices[i].x);
        meshWriter.Write(pMesh->mVertices[i].y);
        meshWriter.Write(pMesh->mVertices[i].z);
        meshWriter.Write(pMesh->mNormals[i].x);
        meshWriter.Write(pMesh->mNormals[i].y);
        meshWriter.Write(pMesh->mNormals[i].z);
        if (pMesh->mTextureCoords[0]) {
            meshWriter.Write(pMesh->mTextureCoords[0][i].x);
            meshWriter.Write(pMesh->mTextureCoords[0][i].y);
        }
        else {
            meshWriter.Write(0.0f);
            meshWriter.Write(0.0f);
        }
		meshWriter.Write(pMesh->mTangents[i].x);
		meshWriter.Write(pMesh->mTangents[i].y);
		meshWriter.Write(pMesh->mTangents[i].z);
		meshWriter.Write(pMesh->mBitangents[i].x);
		meshWriter.Write(pMesh->mBitangents[i].y);
		meshWriter.Write(pMesh->mBitangents[i].z);
    }

    //インデックスの処理
    meshWriter.Write(pMesh->mNumFaces);
    for (UINT i = 0; i < pMesh->mNumFaces; i++) {
        aiFace face = pMesh->mFaces[i];
        meshWriter.Write(face.mNumIndices);
        for (UINT j = 0; j < face.mNumIndices; j++) {
            meshWriter.Write(face.mIndices[j]);
        }
    }

    meshWriter.Write(pMesh->mNumBones);
    for (UINT i = 0; i < pMesh->mNumBones; i++) {
        aiBone* bone = pMesh->mBones[i];
        std::string boneName = bone->mName.C_Str();
        meshWriter.Write(boneName);
        //ボーンの影響を受ける頂点の数ぶん繰り返す
        meshWriter.Write(bone->mNumWeights);
        for (UINT j = 0; j < bone->mNumWeights; j++) {
            //頂点インデックス
            meshWriter.Write(bone->mWeights[j].mVertexId);
            //その頂点がどのくらい影響を受けるか
            meshWriter.Write(bone->mWeights[j].mWeight);
        }
    }

    std::vector<char> meshBuffer = meshWriter.GetBuffer();
    WriteCompression(meshBuffer);
}

void ConvertFromFBX::ProcessBone(Character* pCharacter)
{
    std::vector<Bone>& bones = pCharacter->m_boneManager.m_bones;
    std::unordered_map<std::string, XMMATRIX>& finalBoneTransforms = pCharacter->m_boneManager.m_finalBoneTransforms;

    BinaryWriter boneWriter;
    boneWriter.OpenWrite();

    boneWriter.Write(static_cast<UINT>(bones.size()));
    for (size_t i = 0; i < bones.size(); i++) {
        Bone& bone = bones[i];
        boneWriter.Write(bone.GetBoneName());
        boneWriter.Write(bone.m_boneOffset);
        char c = bone.m_bType;
        boneWriter.Write(&c, 1);
        boneWriter.Write(finalBoneTransforms[bone.GetBoneName()]);
    };
    for (size_t i = 0; i < bones.size(); i++) {
        Bone& bone = bones[i];
        boneWriter.Write(bone.GetParentBoneIndex());
        boneWriter.Write((BYTE)bone.GetChildBoneCount());

        for (UINT j = 0; j < bone.GetChildBoneCount(); j++) {
            boneWriter.Write(bone.GetChildBoneIndex(j));
        }
    }

	std::vector<char> boneBuffer = boneWriter.GetBuffer();
    WriteCompression(boneBuffer);
}

void ConvertFromFBX::ProcessHumanMesh(Character* pCharacter)
{
    //HumanoidMeshの内容を保存

    std::vector<Character::HumanoidMesh>& humanoidMeshes = pCharacter->GetHumanMeshes();

    BinaryWriter humanoidWriter;
    humanoidWriter.OpenWrite();

    humanoidWriter.Write(static_cast<UINT>(humanoidMeshes.size()));
    for (size_t i = 0; i < humanoidMeshes.size(); i++) {
        Character::HumanoidMesh& humanoidMesh = humanoidMeshes[i];

        //メッシュ名
        humanoidWriter.Write(humanoidMesh.meshName);

        //shapeMapping
        std::unordered_map<std::string, UINT>& shapeMappings = humanoidMesh.shapeMapping;
        humanoidWriter.Write(static_cast<UINT>(shapeMappings.size()));
        for (std::pair<std::string, UINT> shapeMapping : shapeMappings) {
            humanoidWriter.Write(shapeMapping.first);
            humanoidWriter.Write(shapeMapping.second);
        }

        //shapeDeltas
        //XMFLOAT3(0.0f ,0.0f, 0.0f)でない要素のみ抽出
		std::vector<UINT> shapeDeltasIndex;
		std::vector<XMFLOAT3> shapeDeltas;

        for (size_t j = 0; j < humanoidMesh.shapeDeltas.size(); j++) {
            XMFLOAT3 shapeDelta = humanoidMesh.shapeDeltas[j];

            //シェイプキーの影響を受けない頂点は書き込まない
            if (abs(shapeDelta.x) > SAVE_SHAPE_DELTA || abs(shapeDelta.y) > SAVE_SHAPE_DELTA || abs(shapeDelta.z) > SAVE_SHAPE_DELTA) {
				shapeDeltasIndex.push_back(static_cast<UINT>(j));
				shapeDeltas.push_back(shapeDelta);
            }
        }

        humanoidWriter.Write(static_cast<UINT>(humanoidMesh.shapeDeltas.size()));
        humanoidWriter.Write(static_cast<UINT>(shapeDeltasIndex.size()));
        for (size_t j = 0; j < shapeDeltasIndex.size(); j++) {
            XMFLOAT3 shapeDelta = shapeDeltas[j];

            humanoidWriter.Write(shapeDeltasIndex[j]);
            humanoidWriter.Write(shapeDelta.x);
            humanoidWriter.Write(shapeDelta.y);
            humanoidWriter.Write(shapeDelta.z);
        }
    }

    std::vector<char> humanoidBuffer = humanoidWriter.GetBuffer();
    WriteCompression(humanoidBuffer);
}

void ConvertFromFBX::WriteCompression(std::vector<char>& originalBuffer)
{
	//データを圧縮して書き込む
    std::vector<char> compressedBuffer;
    BinaryCompress(originalBuffer.data(), originalBuffer.size(), compressedBuffer, COMPRESSION_LEVEL);

    bw.Write(static_cast<UINT>(originalBuffer.size()));
    bw.Write(static_cast<UINT>(compressedBuffer.size()));
    bw.Write(compressedBuffer.data(), compressedBuffer.size());
}
