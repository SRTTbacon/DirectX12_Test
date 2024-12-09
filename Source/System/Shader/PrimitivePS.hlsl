#define BIAS 0.0001f     //�V���M�[�}���̃o�C�A�X�l

SamplerState texSampler : register(s0); //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1); //�e�N�X�`���p�T���v���[
Texture2D<float> shadowMap : register(t1); //�V���h�E�}�b�v

//�萔�o�b�t�@
cbuffer LightBuffer : register(b0)
{
    float3 lightDir;        //���C�g�̕���
    float3 ambientColor;
    float3 diffuseColor;
};

struct VSOutput
{
    float4 svpos : SV_POSITION; //���_�V�F�[�_�[���痈�����W
    float3 normal : NORMAL; //���_�V�F�[�_�[����k�m�[�}���}�b�v
    float2 uv : TEXCOORD; //���_�V�F�[�_�[���痈��UV
    float4 shadowPos : TEXCOORD1;
};

float ShadowCalculation(float4 shadowPos)
{
    // 1. �ˉe���W�ɕϊ�
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy *= float2(1.0f, -1.0f);
    projCoords = projCoords * 0.5f + 0.5f;

    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f; // �͈͊O�͉e�Ȃ�
    }

    // 2. �u���b�J�[�T��
    float texelSize = 1.0f / 8192.0f;
    float averageBlockerDepth = 0.0f;
    int blockerCount = 0;

    int searchRadius = 3; // �u���b�J�[�T���͈�
    for (int x = -searchRadius; x <= searchRadius; x++)
    {
        for (int y = -searchRadius; y <= searchRadius; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            float shadowDepth = shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy + offset, projCoords.z - BIAS);
            if (shadowDepth < projCoords.z)
            {
                averageBlockerDepth += shadowDepth;
                blockerCount++;
            }
        }
    }

    if (blockerCount == 0)
    {
        return 1.0f; // �e�Ȃ�
    }

    averageBlockerDepth /= blockerCount;

    // 3. ���e�T�C�Y�̌v�Z
    float penumbraSize = (projCoords.z - averageBlockerDepth) / averageBlockerDepth * 0.05f;

    // 4. PCF�ɂ��V���h�E�T���v�����O
    int filterRadius = saturate(penumbraSize / texelSize);
    float shadowFactor = 0.0f;
    int samples = 0;

    for (int x1 = -filterRadius; x1 <= filterRadius; x1++)
    {
        for (int y = -filterRadius; y <= filterRadius; y++)
        {
            float2 offset = float2(x1, y) * texelSize;
            shadowFactor += shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy + offset, projCoords.z - BIAS);
            samples++;
        }
    }

    return shadowFactor / samples;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //���C�g�̕������t��
    float lightIntensity = saturate(dot(input.normal, -lightDir));

    // �V���h�E�̌v�Z
    float shadowFactor = ShadowCalculation(input.shadowPos);
    
    float3 diffuse = float3(1.0f, 1.0f, 1.0f) * lightIntensity;

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float3 lighting = ambientColor + shadowFactor * diffuse;

    //���C�e�B���O���ʂƃe�N�X�`���J���[���|�����킹��
    float4 finalColor = float4(lighting, 1.0f);

    return finalColor;
}