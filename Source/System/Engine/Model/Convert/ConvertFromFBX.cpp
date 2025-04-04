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
        ProcessMesh(mesh, true);
    }

    //各メッシュのマテリアル(テクスチャ)情報
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

ConvertResult ConvertFromFBX::ConvertFromModel(std::string fromFilePath, std::string toFilePath)
{
    if (!std::filesystem::exists(fromFilePath)) {
        return ConvertResult::FileNotExist;
    }

    if (!bw.OpenWrite(toFilePath)) {
        return ConvertResult::CantWriteFile;
    }

    Assimp::Importer importer;

    //ReadFileだと日本語のパスが入っていると読み込めないため、ifstreamで読み込んで渡す
    std::ifstream file(fromFilePath, std::ios::binary);
    if (!file) {
        printf("モデルファイルが開けません: %s\n", fromFilePath.c_str());
        return ConvertResult::FileNotExist;
    }

    std::string extension = GetFileExtension(fromFilePath);

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    //モデル読み込み時のフラグ。メッシュのポリゴンはすべて三角形にし、ボーンが存在する場合は影響を受けるウェイトを4つまでにする
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
    const aiScene* scene = importer.ReadFileFromMemory(buffer.data(), buffer.size(), flag, extension.c_str());

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return ConvertResult::FbxFormatExeption;
    }

    //ヘッダー
    bw.Write((BYTE)std::strlen(MODEL_HEADER));
    bw.Write(MODEL_HEADER, std::strlen(MODEL_HEADER));
    bw.Write(MODEL_DEFAULT);

    //メッシュ情報を変換
    bw.Write(scene->mNumMeshes);
    ProcessNode(scene, scene->mRootNode);

    ProcessAnimation(scene);

    return ConvertResult::Success;
}

void ConvertFromFBX::ProcessNode(const aiScene* pScene, const aiNode* pNode)
{
    //ノードにメッシュが含まれていたら読み込む
    for (UINT j = 0; j < pNode->mNumMeshes; j++) {
        aiMesh* pAiMesh = pScene->mMeshes[pNode->mMeshes[j]];
        ProcessMesh(pAiMesh, false, pNode);
    }

    for (UINT i = 0; i < pNode->mNumChildren; i++) {
        aiNode* childNode = pNode->mChildren[i];
        ProcessNode(pScene, childNode);
    }
}

void ConvertFromFBX::ProcessMesh(const aiMesh* pMesh, bool bIncludeBone, const aiNode* pNode)
{
    BinaryWriter meshWriter;
    meshWriter.OpenWrite();

    //頂点の処理
    meshWriter.Write(pMesh->mNumVertices);      //頂点数
    for (UINT i = 0; i < pMesh->mNumVertices; i++) {
        //頂点の位置
        meshWriter.Write(pMesh->mVertices[i].x);
        meshWriter.Write(pMesh->mVertices[i].y);
        meshWriter.Write(pMesh->mVertices[i].z);
        //法線
        meshWriter.Write(pMesh->mNormals[i].x);
        meshWriter.Write(pMesh->mNormals[i].y);
        meshWriter.Write(pMesh->mNormals[i].z);
        //UVが存在すればUV座標
        if (pMesh->mTextureCoords[0]) {
            meshWriter.Write(pMesh->mTextureCoords[0][i].x);
            meshWriter.Write(pMesh->mTextureCoords[0][i].y);
        }
        else {
            meshWriter.Write(0.0f);
            meshWriter.Write(0.0f);
        }
        //従法線
		meshWriter.Write(pMesh->mTangents[i].x);
		meshWriter.Write(pMesh->mTangents[i].y);
		meshWriter.Write(pMesh->mTangents[i].z);
        //従接線
		meshWriter.Write(pMesh->mBitangents[i].x);
		meshWriter.Write(pMesh->mBitangents[i].y);
		meshWriter.Write(pMesh->mBitangents[i].z);
    }

    //インデックスの処理
    meshWriter.Write(pMesh->mNumFaces);
    for (UINT i = 0; i < pMesh->mNumFaces; i++) {
        aiFace face = pMesh->mFaces[i];
        meshWriter.Write(face.mNumIndices);     //インデックス数
        for (UINT j = 0; j < face.mNumIndices; j++) {
            meshWriter.Write(face.mIndices[j]); //インデックス
        }
    }

    if (bIncludeBone) {
        //ボーン
        meshWriter.Write(pMesh->mNumBones);         //ボーン数
        for (UINT i = 0; i < pMesh->mNumBones; i++) {
            aiBone* bone = pMesh->mBones[i];

            std::string boneName = bone->mName.C_Str();
            meshWriter.Write(boneName);     //ボーン名

            //ボーンの影響を受ける頂点の数ぶん繰り返す
            meshWriter.Write(bone->mNumWeights);    //ウェイト数
            for (UINT j = 0; j < bone->mNumWeights; j++) {
                //頂点インデックス
                meshWriter.Write(bone->mWeights[j].mVertexId);
                //その頂点がどのくらい影響を受けるか
                meshWriter.Write(bone->mWeights[j].mWeight);
            }
        }
    }

    if (pNode) {
        std::string meshName = UTF8ToShiftJIS(pNode->mName.C_Str());

        //初期位置、角度を取得
        XMMATRIX matrix = GetMeshDefaultMatrix(pNode);
        XMFLOAT3 m_position = XMFLOAT3(matrix.r[3].m128_f32[0], matrix.r[3].m128_f32[1], matrix.r[3].m128_f32[2]) / 100.0f;
        XMVECTOR rotation = ExtractEulerAngles(matrix);
        XMFLOAT3 m_rotation = XMFLOAT3(XMVectorGetX(rotation), XMVectorGetY(rotation), XMVectorGetZ(rotation));

        meshWriter.Write(meshName);
        meshWriter.Write(m_position);
        meshWriter.Write(m_rotation);
    }

    //圧縮して保存
    std::vector<char> meshBuffer = meshWriter.GetBuffer();
    WriteCompression(meshBuffer);
}

void ConvertFromFBX::ProcessBone(Character* pCharacter)
{
    //ボーン情報を保存

    //すべてのボーンを取得
    std::vector<Bone>& bones = pCharacter->GetBoneManager()->m_bones;

    BinaryWriter boneWriter;
    boneWriter.OpenWrite();

    //ボーン数
    boneWriter.Write(static_cast<UINT>(bones.size()));
    for (size_t i = 0; i < bones.size(); i++) {
        Bone& bone = bones[i];
        boneWriter.Write(bone.GetBoneName());   //ボーン名
        boneWriter.Write(bone.GetBoneOffset()); //ボーンの初期化時のマトリックス
        char type = bone.m_boneType;            //ボーンのタイプ(左腕や右足など)
        boneWriter.Write(&type, 1);
        boneWriter.Write(bone.GetBoneOffset());    //ボーンの初期化時のマトリックス
    };
    for (size_t i = 0; i < bones.size(); i++) {
        Bone& bone = bones[i];
        boneWriter.Write(bone.GetParentBoneIndex());            //親ボーンのインデックス
        boneWriter.Write((BYTE)bone.GetChildBoneCount());       //子ボーン数

        for (UINT j = 0; j < bone.GetChildBoneCount(); j++) {
            boneWriter.Write(bone.GetChildBoneIndex(j));    //子ボーンのインデックス
        }
    }

    //圧縮して保存
	std::vector<char> boneBuffer = boneWriter.GetBuffer();
    WriteCompression(boneBuffer);
}

void ConvertFromFBX::ProcessHumanMesh(Character* pCharacter)
{
    //HumanoidMeshの内容を保存

    std::vector<Character::HumanoidMesh>& humanoidMeshes = pCharacter->GetHumanMeshes();

    BinaryWriter humanoidWriter; 
    humanoidWriter.OpenWrite();

    //メッシュ数
    humanoidWriter.Write(static_cast<UINT>(humanoidMeshes.size()));
    for (size_t i = 0; i < humanoidMeshes.size(); i++) {
        Character::HumanoidMesh& humanoidMesh = humanoidMeshes[i];

        //メッシュ名
        humanoidWriter.Write(humanoidMesh.pMesh->meshName);

        //shapeMapping(シェイプキーの名前とインデックスを紐づけ)
        std::unordered_map<std::string, UINT>& shapeMappings = humanoidMesh.shapeMapping;
        humanoidWriter.Write(static_cast<UINT>(shapeMappings.size()));
        for (std::pair<std::string, UINT> shapeMapping : shapeMappings) {
            humanoidWriter.Write(shapeMapping.first);   //名前
            humanoidWriter.Write(shapeMapping.second);  //インデックス
        }

        //shapeDeltas (適応後の各頂点の相対座標)
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
            humanoidWriter.Write(shapeDelta);
        }
    }

    //圧縮して保存
    std::vector<char> humanoidBuffer = humanoidWriter.GetBuffer();
    WriteCompression(humanoidBuffer);
}

void ConvertFromFBX::ProcessAnimation(const aiScene* pScene)
{
    BinaryWriter animBuffer;
    animBuffer.OpenWrite();

    animBuffer.Write(pScene->mNumAnimations);

    for (UINT i = 0; i < pScene->mNumAnimations; i++) {
        aiAnimation* pAnim = pScene->mAnimations[i];

        //1秒が何チックか
        double ticksPerSecond = (pAnim->mTicksPerSecond != 0) ? pAnim->mTicksPerSecond : 30.0;
        animBuffer.Write(static_cast<float>(ticksPerSecond));
        double durationInSeconds = pAnim->mDuration / ticksPerSecond;
        animBuffer.Write(static_cast<float>(durationInSeconds));

        animBuffer.Write(pAnim->mNumChannels);

        for (UINT j = 0; j < pAnim->mNumChannels; j++) {
            aiNodeAnim* pChannel = pAnim->mChannels[j];

            std::string meshName = UTF8ToShiftJIS(pChannel->mNodeName.C_Str());

            animBuffer.Write(meshName);

            animBuffer.Write(pChannel->mNumPositionKeys);
            animBuffer.Write(pChannel->mNumRotationKeys);
            animBuffer.Write(pChannel->mNumScalingKeys);

            for (UINT k = 0; k < pChannel->mNumPositionKeys; k++) {
                XMFLOAT3 value = XMFLOAT3(pChannel->mPositionKeys[k].mValue.x, pChannel->mPositionKeys[k].mValue.y, pChannel->mPositionKeys[k].mValue.z);
                float timeInSeconds = static_cast<float>(pChannel->mPositionKeys[k].mTime / ticksPerSecond);
                animBuffer.Write(timeInSeconds);
                animBuffer.Write(value);
            }
            for (UINT k = 0; k < pChannel->mNumRotationKeys; k++) {
                aiQuaternion& q = pChannel->mRotationKeys[k].mValue;
                //クオータニオンの成分
                double x = q.x;
                double y = q.y;
                double z = q.z;
                double w = q.w;

                //Pitch (X軸回転)
                double sinPitch = 2.0 * (w * x + y * z);
                double cosPitch = 1.0 - 2.0 * (x * x + y * y);
                float pitch = static_cast<float>(std::atan2(sinPitch, cosPitch));

                //Yaw (Y軸回転)
                double sinYaw = 2.0 * (w * y - z * x);
                sinYaw = std::clamp(sinYaw, -1.0, 1.0); // asinの範囲外対策
                float yaw = static_cast<float>(std::asin(sinYaw));

                //Roll (Z軸回転)
                double sinRoll = 2.0 * (w * z + x * y);
                double cosRoll = 1.0 - 2.0 * (y * y + z * z);
                float roll = static_cast<float>(std::atan2(sinRoll, cosRoll));

                float timeInSeconds = static_cast<float>(pChannel->mRotationKeys[k].mTime / ticksPerSecond);

                animBuffer.Write(timeInSeconds);
                animBuffer.Write(pitch);
                animBuffer.Write(yaw);
                animBuffer.Write(roll);
            }
            for (UINT k = 0; k < pChannel->mNumScalingKeys; k++) {
                XMFLOAT3 value = XMFLOAT3(pChannel->mScalingKeys[k].mValue.x, pChannel->mScalingKeys[k].mValue.y, pChannel->mScalingKeys[k].mValue.z);
                float timeInSeconds = static_cast<float>(pChannel->mScalingKeys[k].mTime / ticksPerSecond);
                animBuffer.Write(timeInSeconds);
                animBuffer.Write(value);
            }
        }
    }

    //圧縮して保存
    std::vector<char> humanoidBuffer = animBuffer.GetBuffer();
    WriteCompression(humanoidBuffer);
}

void ConvertFromFBX::WriteCompression(std::vector<char>& originalBuffer)
{
	//データを圧縮して書き込む
    std::vector<char> compressedBuffer;
    BinaryCompress(originalBuffer.data(), originalBuffer.size(), compressedBuffer);

    bw.Write(static_cast<UINT>(originalBuffer.size()));         //圧縮前のサイズ (展開時に計算する処理を省くためあらかじめ保存)
    bw.Write(static_cast<UINT>(compressedBuffer.size()));       //圧縮後のサイズ
    bw.Write(compressedBuffer.data(), compressedBuffer.size()); //圧縮後のデータ
}

DirectX::XMMATRIX ConvertFromFBX::GetMeshDefaultMatrix(const aiNode* pNode)
{
    XMMATRIX parentMatrix = XMMatrixIdentity();
    if (pNode->mParent != nullptr) {
        parentMatrix = GetMeshDefaultMatrix(pNode->mParent);
    }

    XMMATRIX currentMatrix = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&pNode->mTransformation)));
    return currentMatrix * parentMatrix;
}
