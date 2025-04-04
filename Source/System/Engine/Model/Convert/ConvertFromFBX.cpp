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

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A�{�[�������݂���ꍇ�͉e�����󂯂�E�F�C�g��4�܂łɂ���
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
    const aiScene* scene = importer.ReadFile(fromFilePath, flag);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return ConvertResult::FbxFormatExeption;
    }

    //�w�b�_�[
    bw.Write((BYTE)std::strlen(MODEL_HEADER));
    bw.Write(MODEL_HEADER, std::strlen(MODEL_HEADER));
    bw.Write(MODEL_CHARACTER);

    //�{�[���ƃV�F�C�v����ۑ�
    ProcessBone(pCharacter);
    ProcessHumanMesh(pCharacter);

    //���b�V������ϊ�
    bw.Write(scene->mNumMeshes);
    for (UINT i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        ProcessMesh(mesh, true);
    }

    //�e���b�V���̃}�e���A��(�e�N�X�`��)���
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        //���b�V���̃}�e���A�����擾����
        bw.Write(mesh->mMaterialIndex >= 0);
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //�t�@�C���� = �}�e���A���� + .png
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

    //ReadFile���Ɠ��{��̃p�X�������Ă���Ɠǂݍ��߂Ȃ����߁Aifstream�œǂݍ���œn��
    std::ifstream file(fromFilePath, std::ios::binary);
    if (!file) {
        printf("���f���t�@�C�����J���܂���: %s\n", fromFilePath.c_str());
        return ConvertResult::FileNotExist;
    }

    std::string extension = GetFileExtension(fromFilePath);

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    //���f���ǂݍ��ݎ��̃t���O�B���b�V���̃|���S���͂��ׂĎO�p�`�ɂ��A�{�[�������݂���ꍇ�͉e�����󂯂�E�F�C�g��4�܂łɂ���
    UINT flag = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
        aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
    const aiScene* scene = importer.ReadFileFromMemory(buffer.data(), buffer.size(), flag, extension.c_str());

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return ConvertResult::FbxFormatExeption;
    }

    //�w�b�_�[
    bw.Write((BYTE)std::strlen(MODEL_HEADER));
    bw.Write(MODEL_HEADER, std::strlen(MODEL_HEADER));
    bw.Write(MODEL_DEFAULT);

    //���b�V������ϊ�
    bw.Write(scene->mNumMeshes);
    ProcessNode(scene, scene->mRootNode);

    ProcessAnimation(scene);

    return ConvertResult::Success;
}

void ConvertFromFBX::ProcessNode(const aiScene* pScene, const aiNode* pNode)
{
    //�m�[�h�Ƀ��b�V�����܂܂�Ă�����ǂݍ���
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

    //���_�̏���
    meshWriter.Write(pMesh->mNumVertices);      //���_��
    for (UINT i = 0; i < pMesh->mNumVertices; i++) {
        //���_�̈ʒu
        meshWriter.Write(pMesh->mVertices[i].x);
        meshWriter.Write(pMesh->mVertices[i].y);
        meshWriter.Write(pMesh->mVertices[i].z);
        //�@��
        meshWriter.Write(pMesh->mNormals[i].x);
        meshWriter.Write(pMesh->mNormals[i].y);
        meshWriter.Write(pMesh->mNormals[i].z);
        //UV�����݂����UV���W
        if (pMesh->mTextureCoords[0]) {
            meshWriter.Write(pMesh->mTextureCoords[0][i].x);
            meshWriter.Write(pMesh->mTextureCoords[0][i].y);
        }
        else {
            meshWriter.Write(0.0f);
            meshWriter.Write(0.0f);
        }
        //�]�@��
		meshWriter.Write(pMesh->mTangents[i].x);
		meshWriter.Write(pMesh->mTangents[i].y);
		meshWriter.Write(pMesh->mTangents[i].z);
        //�]�ڐ�
		meshWriter.Write(pMesh->mBitangents[i].x);
		meshWriter.Write(pMesh->mBitangents[i].y);
		meshWriter.Write(pMesh->mBitangents[i].z);
    }

    //�C���f�b�N�X�̏���
    meshWriter.Write(pMesh->mNumFaces);
    for (UINT i = 0; i < pMesh->mNumFaces; i++) {
        aiFace face = pMesh->mFaces[i];
        meshWriter.Write(face.mNumIndices);     //�C���f�b�N�X��
        for (UINT j = 0; j < face.mNumIndices; j++) {
            meshWriter.Write(face.mIndices[j]); //�C���f�b�N�X
        }
    }

    if (bIncludeBone) {
        //�{�[��
        meshWriter.Write(pMesh->mNumBones);         //�{�[����
        for (UINT i = 0; i < pMesh->mNumBones; i++) {
            aiBone* bone = pMesh->mBones[i];

            std::string boneName = bone->mName.C_Str();
            meshWriter.Write(boneName);     //�{�[����

            //�{�[���̉e�����󂯂钸�_�̐��Ԃ�J��Ԃ�
            meshWriter.Write(bone->mNumWeights);    //�E�F�C�g��
            for (UINT j = 0; j < bone->mNumWeights; j++) {
                //���_�C���f�b�N�X
                meshWriter.Write(bone->mWeights[j].mVertexId);
                //���̒��_���ǂ̂��炢�e�����󂯂邩
                meshWriter.Write(bone->mWeights[j].mWeight);
            }
        }
    }

    if (pNode) {
        std::string meshName = UTF8ToShiftJIS(pNode->mName.C_Str());

        //�����ʒu�A�p�x���擾
        XMMATRIX matrix = GetMeshDefaultMatrix(pNode);
        XMFLOAT3 m_position = XMFLOAT3(matrix.r[3].m128_f32[0], matrix.r[3].m128_f32[1], matrix.r[3].m128_f32[2]) / 100.0f;
        XMVECTOR rotation = ExtractEulerAngles(matrix);
        XMFLOAT3 m_rotation = XMFLOAT3(XMVectorGetX(rotation), XMVectorGetY(rotation), XMVectorGetZ(rotation));

        meshWriter.Write(meshName);
        meshWriter.Write(m_position);
        meshWriter.Write(m_rotation);
    }

    //���k���ĕۑ�
    std::vector<char> meshBuffer = meshWriter.GetBuffer();
    WriteCompression(meshBuffer);
}

void ConvertFromFBX::ProcessBone(Character* pCharacter)
{
    //�{�[������ۑ�

    //���ׂẴ{�[�����擾
    std::vector<Bone>& bones = pCharacter->GetBoneManager()->m_bones;

    BinaryWriter boneWriter;
    boneWriter.OpenWrite();

    //�{�[����
    boneWriter.Write(static_cast<UINT>(bones.size()));
    for (size_t i = 0; i < bones.size(); i++) {
        Bone& bone = bones[i];
        boneWriter.Write(bone.GetBoneName());   //�{�[����
        boneWriter.Write(bone.GetBoneOffset()); //�{�[���̏��������̃}�g���b�N�X
        char type = bone.m_boneType;            //�{�[���̃^�C�v(���r��E���Ȃ�)
        boneWriter.Write(&type, 1);
        boneWriter.Write(bone.GetBoneOffset());    //�{�[���̏��������̃}�g���b�N�X
    };
    for (size_t i = 0; i < bones.size(); i++) {
        Bone& bone = bones[i];
        boneWriter.Write(bone.GetParentBoneIndex());            //�e�{�[���̃C���f�b�N�X
        boneWriter.Write((BYTE)bone.GetChildBoneCount());       //�q�{�[����

        for (UINT j = 0; j < bone.GetChildBoneCount(); j++) {
            boneWriter.Write(bone.GetChildBoneIndex(j));    //�q�{�[���̃C���f�b�N�X
        }
    }

    //���k���ĕۑ�
	std::vector<char> boneBuffer = boneWriter.GetBuffer();
    WriteCompression(boneBuffer);
}

void ConvertFromFBX::ProcessHumanMesh(Character* pCharacter)
{
    //HumanoidMesh�̓��e��ۑ�

    std::vector<Character::HumanoidMesh>& humanoidMeshes = pCharacter->GetHumanMeshes();

    BinaryWriter humanoidWriter; 
    humanoidWriter.OpenWrite();

    //���b�V����
    humanoidWriter.Write(static_cast<UINT>(humanoidMeshes.size()));
    for (size_t i = 0; i < humanoidMeshes.size(); i++) {
        Character::HumanoidMesh& humanoidMesh = humanoidMeshes[i];

        //���b�V����
        humanoidWriter.Write(humanoidMesh.pMesh->meshName);

        //shapeMapping(�V�F�C�v�L�[�̖��O�ƃC���f�b�N�X��R�Â�)
        std::unordered_map<std::string, UINT>& shapeMappings = humanoidMesh.shapeMapping;
        humanoidWriter.Write(static_cast<UINT>(shapeMappings.size()));
        for (std::pair<std::string, UINT> shapeMapping : shapeMappings) {
            humanoidWriter.Write(shapeMapping.first);   //���O
            humanoidWriter.Write(shapeMapping.second);  //�C���f�b�N�X
        }

        //shapeDeltas (�K����̊e���_�̑��΍��W)
        //XMFLOAT3(0.0f ,0.0f, 0.0f)�łȂ��v�f�̂ݒ��o
		std::vector<UINT> shapeDeltasIndex;
		std::vector<XMFLOAT3> shapeDeltas;

        for (size_t j = 0; j < humanoidMesh.shapeDeltas.size(); j++) {
            XMFLOAT3 shapeDelta = humanoidMesh.shapeDeltas[j];

            //�V�F�C�v�L�[�̉e�����󂯂Ȃ����_�͏������܂Ȃ�
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

    //���k���ĕۑ�
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

        //1�b�����`�b�N��
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
                //�N�I�[�^�j�I���̐���
                double x = q.x;
                double y = q.y;
                double z = q.z;
                double w = q.w;

                //Pitch (X����])
                double sinPitch = 2.0 * (w * x + y * z);
                double cosPitch = 1.0 - 2.0 * (x * x + y * y);
                float pitch = static_cast<float>(std::atan2(sinPitch, cosPitch));

                //Yaw (Y����])
                double sinYaw = 2.0 * (w * y - z * x);
                sinYaw = std::clamp(sinYaw, -1.0, 1.0); // asin�͈̔͊O�΍�
                float yaw = static_cast<float>(std::asin(sinYaw));

                //Roll (Z����])
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

    //���k���ĕۑ�
    std::vector<char> humanoidBuffer = animBuffer.GetBuffer();
    WriteCompression(humanoidBuffer);
}

void ConvertFromFBX::WriteCompression(std::vector<char>& originalBuffer)
{
	//�f�[�^�����k���ď�������
    std::vector<char> compressedBuffer;
    BinaryCompress(originalBuffer.data(), originalBuffer.size(), compressedBuffer);

    bw.Write(static_cast<UINT>(originalBuffer.size()));         //���k�O�̃T�C�Y (�W�J���Ɍv�Z���鏈�����Ȃ����߂��炩���ߕۑ�)
    bw.Write(static_cast<UINT>(compressedBuffer.size()));       //���k��̃T�C�Y
    bw.Write(compressedBuffer.data(), compressedBuffer.size()); //���k��̃f�[�^
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
