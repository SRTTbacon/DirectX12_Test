//�e�݂̂̃V�F�[�_�[

//�o�b�t�@���g���܂킷����BoneVS�Ɠ������e
cbuffer TransformBuffer : register(b0)
{
    matrix modelMatrix; //���f���}�g���b�N�X
    matrix viewMatrix; //�r���[�}�g���b�N�X
    matrix projectionMatrix; //�v���W�F�N�V�����}�g���b�N�X
    matrix lightViewProjMatrix; //�f�B���N�V���i�����C�g�̏��
    matrix normalMatrix; //���f���̃X�P�[���A��]�Ȃǂ�input.normal�ɂ��K������p
    float4 eyePos;              //�J�����̈ʒu
};

cbuffer BoneMatrices : register(b1)
{
    matrix boneMatrices[512]; //�{�[���}�g���b�N�X�i�ő�512��)
}

cbuffer Constants : register(b2) //����������1�x�����������s���Ȃ�����
{
    uint vertexCount; //���_��
    uint shapeCount; //�V�F�C�v�L�[�̐�
};

struct VSInput
{
    float3 position : POSITION;         //���_���W
    float4 boneWeights : BONEWEIGHTS;   //�e���_�̃{�[���̉e���x (�{�[�������݂��Ȃ��ꍇ�͂��ׂ�0)
    uint4 boneIDs : BONEIDS;            //�e���_�ɉe����^����{�[���̃C���f�b�N�X (�{�[�������݂��Ȃ��ꍇ�͂��ׂ�0)
    uint vertexID : VERTEXID;           //���_��ID
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

Texture2D<float4> ShapeDeltasTexture : register(t0); //�V�F�C�v�L�[���Ƃ̈ʒu�ψʃf�[�^
StructuredBuffer<float> ShapeWeights : register(t1); //�e�V�F�C�v�L�[�̃E�F�C�g

//���_ID�ƃV�F�C�v�L�[��ID���瑊�Έʒu���擾
float3 GetShapeDelta(uint vertexID, uint shapeID)
{
    //�E�F�C�g�����ɏ������ꍇ�͖���
    float weight = ShapeWeights[shapeID];
    if (weight < 0.001f)
        return float3(0.0f, 0.0f, 0.0f);
    
    //�V�F�C�v�L�[�̃e�N�X�`���T�C�Y���擾
    uint width = 0, height = 0;
    ShapeDeltasTexture.GetDimensions(width, height);

    //���W
    uint x = vertexID % width;
    uint y = (vertexID / width) * shapeCount + shapeID;

    return ShapeDeltasTexture.Load(int3(x, y, 0)).xyz * weight;
}

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;

    //�V�F�C�v�L�[��K��(���݂����)
    float3 shapePos = input.position;
    for (uint i = 0; i < shapeCount; i++)
    {
        shapePos += GetShapeDelta(input.vertexID, i);
    }

    float4 localPos = float4(0.0f, 0.0f, 0.0f, 0.0f);

    //�{�[����K��(���݂����)
    if (input.boneWeights.x == 0.0f && input.boneWeights.y == 0.0f && input.boneWeights.z == 0.0f && input.boneWeights.w == 0.0f)
    {
        localPos = float4(shapePos, 1.0f); //���_���W
    }
    else
    {
        //�{�[���𔽉f
        matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

        localPos = mul(float4(shapePos, 1.0f), skinMatrix); //���_���W
    }
    float4 worldPos = mul(modelMatrix, localPos);       //���[���h���W�ɕϊ�
    output.position = mul(lightViewProjMatrix, worldPos);
    return output;
}