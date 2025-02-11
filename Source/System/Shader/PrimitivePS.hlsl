#define BIAS 0.0001f     //�V���M�[�}���̃o�C�A�X�l
#define SHADOWSIZE 8192.0f

SamplerState texSampler : register(s0);         //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1); //�e�N�X�`���p�T���v���[
Texture2D<float4> _MainTex : register(t0);      //�e�N�X�`��
Texture2D<float4> _NormalMap : register(t1);    //�m�[�}���}�b�v
Texture2D<float> shadowMap : register(t2);      //�V���h�E�}�b�v

//�萔�o�b�t�@
cbuffer LightBuffer : register(b0)
{
    float4 lightDir;        //���C�g�̕���
    float4 ambientColor;    //�e�F
    float4 diffuseColor;    //�W���̐F
    float4 cameraEyePos;    //�J�����̈ʒu
};

struct VSOutput
{
    float4 svpos : SV_POSITION;     //���_�V�F�[�_�[���痈�����W
    float3 normal : NORMAL;         //���_�V�F�[�_�[����k�m�[�}���}�b�v
    float2 uv : TEXCOORD;           //���_�V�F�[�_�[���痈��UV
    float4 shadowPos : TEXCOORD1;   //�e�̈ʒu
};

float IsUVOutOfRange(float2 uv)
{
    float uBelowZero = 1.0 - step(0.0, uv.x);
    float uAboveOne = step(1.0, uv.x);

    float vBelowZero = 1.0 - step(0.0, uv.y);
    float vAboveOne = step(1.0, uv.y);

    float isOutOfRange = uBelowZero + uAboveOne + vBelowZero + vAboveOne;
    return saturate(isOutOfRange);
}

float ShadowCalculation(float4 worldPos, float4 shadowPos)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);
    
    //�e�̏_�炩�������肷�邽�߂̕�����(�T���v�����O��)
    int numSamples = 4; //�T���v���� (�����قǊ��炩�����d��)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
    // �����ɉ������o�C�A�X
    float dynamicBias = lerp(BIAS, 0.001f, saturate(length(worldPos - cameraEyePos) / 50.0f));

    //�V���h�E�}�b�v��ł̃T���v�����O
    for (int i = -numSamples / 2; i < numSamples / 2; ++i)
    {
        for (int j = -numSamples / 2; j < numSamples / 2; ++j)
        {
            //�T���v���̈ʒu
            float2 sampleOffset = float2(i, j) * texelSize;

            //�T���v�����O�ʒu��UV���W
            float2 shadowSampleUV = shadowUV + sampleOffset;
            //shadowSampleUV = saturate(shadowSampleUV);

            //�V���h�E�}�b�v����[�x���T���v�����O
            float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - dynamicBias);
            
            //�T���v�������v
            total += sampleDepth;
        }
    }

    
    //���ς����
    float shadowFactor = total / (numSamples * numSamples);
    //shadowFactor = lerp(shadowFactor, 1.0f, IsUVOutOfRange(shadowUV));

    //�V���h�E�̗L��������(0.0f�Ȃ犮�S�ȉe�A1.0f�Ȃ�e�Ȃ�)
    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //���C�g�̕������t��
    float lightIntensity = saturate(dot(input.normal, -lightDir.xyz));

    //�V���h�E�̌v�Z
    float shadowFactor = ShadowCalculation(input.svpos, input.shadowPos);
    
    float4 diffuse = diffuseColor * lightIntensity;

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float4 lighting = ambientColor + shadowFactor * diffuse;

    return lighting;
}