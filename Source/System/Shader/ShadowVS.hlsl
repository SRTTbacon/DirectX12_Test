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

struct VSInput
{
    float3 position : POSITION;         //���_���W
    float4 boneWeights : BONEWEIGHTS;   //�e���_�̃{�[���̉e���x (�{�[�������݂��Ȃ��ꍇ�͂��ׂ�0)
    uint4 boneIDs : BONEIDS;            //�e���_�ɉe����^����{�[���̃C���f�b�N�X (�{�[�������݂��Ȃ��ꍇ�͂��ׂ�0)
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    //�{�[���𔽉f
    matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];
    
    float4 localPos = mul(float4(input.position, 1.0f), skinMatrix); //���_���W
    float4 worldPos = mul(modelMatrix, localPos);       //���[���h���W�ɕϊ�
    output.position = mul(lightViewProjMatrix, worldPos);
    return output;
}