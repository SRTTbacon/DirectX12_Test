//�e�݂̂̃V�F�[�_�[

//�o�b�t�@���g���܂킷����BoneVS�Ɠ������e
cbuffer TransformBuffer : register(b0)
{
    matrix modelMatrix; //���f���}�g���b�N�X
    matrix viewMatrix; //�r���[�}�g���b�N�X
    matrix projectionMatrix; //�v���W�F�N�V�����}�g���b�N�X
    matrix lightViewProjMatrix; //�f�B���N�V���i�����C�g�̏��
    matrix normalMatrix; //���f���̃X�P�[���A��]�Ȃǂ�input.normal�ɂ��K������p
};

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), modelMatrix);
    output.position = mul(worldPosition, lightViewProjMatrix);
    return output;
}