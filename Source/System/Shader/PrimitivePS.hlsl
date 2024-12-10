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
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    //�e�̏_�炩�������肷�邽�߂̕�����(�T���v�����O��)
    int numSamples = 4; //�T���v���� (�����قǊ��炩�����d��)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / 8192.0f, 1.0f / 8192.0f);
    
    //�V���h�E�}�b�v��ł̃T���v�����O
    for (int i = -numSamples / 2; i < numSamples / 2; ++i)
    {
        for (int j = -numSamples / 2; j < numSamples / 2; ++j)
        {
            //�T���v���̈ʒu
            float2 sampleOffset = float2(i, j) * texelSize;

            //�T���v�����O�ʒu��UV���W
            float2 shadowSampleUV = shadowUV + sampleOffset;

            //�V���h�E�}�b�v����[�x���T���v�����O
            float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - BIAS);
            
            //�T���v�������v
            total += sampleDepth;
        }
    }

    //���ς����
    float shadowFactor = total / (numSamples * numSamples);

    //�V���h�E�̗L��������(0.0f�Ȃ犮�S�ȉe�A1.0f�Ȃ�e�Ȃ�)
    return shadowFactor;
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