//�J�������
cbuffer Camera : register(b0)
{
    float3 cameraPosition;
    float3 cameraForward;
    float3 cameraRight;
    float3 cameraUp;
};

//�������
cbuffer DirectionalLight : register(b1)
{
    float3 lightDirection;
    float3 lightColor;
};

//�{�[�����
cbuffer Bones : register(b2)
{
    row_major float4x4 boneMatrices[512];
};

//�}�e���A�����
struct Material
{
    float3 diffuseColor;
    float3 specularColor;
    float specularPower;
};

//�����i�e�W�I���g������n�����f�[�^�j
struct Attributes
{
    float3 localPosition;
    float4 boneWeights;
    uint4 boneIndices;
    float3 normal;
    float2 uv;
    uint vertexID;
};

//�y�C���[�h�i���C�̌��ʂ�ێ�����\���́j
struct RayPayload
{
    float3 color;
};

//UV���W���X�N���[����Ԃ���v�Z
float2 ComputeScreenUV(uint2 pixelIndex, uint2 screenDimensions)
{
    return (float2(pixelIndex) + 0.5) / float2(screenDimensions);
}

//���[���h���W�ɂ����郌�C�������v�Z
float3 ComputeWorldRayDirection(float2 screenUV, float3 cameraPos, float3 forward, float3 right, float3 up)
{
    float3 rayDir = normalize(forward + (screenUV.x - 0.5) * 2.0 * right + (screenUV.y - 0.5) * -2.0 * up);
    return rayDir;
}
