#define SHADOWSIZE 8192.0f

//�T���v���[�ƃe�N�X�`��
SamplerState texSampler : register(s0); //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1); //�e�N�X�`���p�T���v���[
Texture2D<float4> _MainTex : register(t0); //�e�N�X�`��
Texture2D<float> _NoiseTex : register(t1); //�e�N�X�`��
Texture2D<float> shadowMap : register(t2); //�V���h�E�}�b�v

//�萔�o�b�t�@
cbuffer PixelBuffer : register(b0)
{
    float4 lightDir;        //���C�g�̕���
    float4 ambientColor;    //�e�F
    float4 diffuseColor;    //�W���̐F
    float4 cameraEyePos;    //�J�����̈ʒu
    float4 fogColor;        //�t�H�O�̐F
    float2 fogStartEnd;     //�t�H�O�̊J�n�����A�I������
};

struct VS_OUTPUT
{
    float4 svpos : SV_POSITION; //�X�N���[�����W
    float3 normal : NORMAL; //�@��
    float2 uv : TEXCOORD; //UV
    float4 shadowPos : TEXCOORD1; //�e�̈ʒu
    float3 worldPos : TEXCOORD2; //���[���h���W
};

static const float2 poissonDisk[8] =
{
    float2(-0.326212, -0.405810),
    float2(-0.840144, -0.073580),
    float2(-0.695914, 0.457137),
    float2(-0.203345, 0.620716),
    float2(0.962340, -0.194983),
    float2(0.473434, -0.480026),
    float2(0.519456, 0.767022),
    float2(0.185461, -0.893124)
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

//�@���ƌ��̕����ɉ������o�C�A�X
float ComputeShadowBias(float3 normal, float3 lightDir)
{
    float cosTheta = saturate(dot(normal, lightDir));
    return max(0.0015 * (1.0 - cosTheta), 0.0005f); //�@�������ƕ��s�Ȃقǃo�C�A�X�����炷
}

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5, -0.5);
    
    float isInside = step(0.0f, shadowUV.x) * step(shadowUV.x, 1.0f) *
                     step(0.0f, shadowUV.y) * step(shadowUV.y, 1.0f);

    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    for (int i = 0; i < 8; ++i)
    {
        float2 sampleOffset = poissonDisk[i] * texelSize * 1.3f;
        float2 shadowSampleUV = shadowUV + sampleOffset;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }

    //���ς����
    float shadowFactor = total / 8.0f;
    
    shadowFactor = lerp(1.0f, shadowFactor, isInside);

    //�V���h�E�̗L��������(0.0f�Ȃ犮�S�ȉe�A1.0f�Ȃ�e�Ȃ�)
    return shadowFactor;
}

// �s�N�Z���V�F�[�_�[
float4 main(VS_OUTPUT input) : SV_TARGET
{
    // ���̃e�N�X�`������F�ƃA���t�@���T���v�����O
    float4 grassTex = _MainTex.Sample(texSampler, input.uv);
    
    // �A���t�@�e�X�g�F�A���t�@�������������͕`�悵�Ȃ�
    if (grassTex.a <= 0.29f) //���������𖳎�
    {
        discard;
    }

    // ���C�e�B���O�v�Z�i�P���ȃf�B�t���[�Y���C�e�B���O�j
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));

    float bias = ComputeShadowBias(input.normal, lightVec);

    //�V���h�E�̌v�Z
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    
    float shadowPower = 1.0f * shadowFactor;
    shadowPower = max(0.1f, shadowPower);

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float4 diffuse = lerp(ambientColor, diffuseColor, shadowPower);

    float2 noiseUV = float2(input.worldPos.x, input.worldPos.z) * -0.02f;
    float noiseTex = _NoiseTex.Sample(texSampler, noiseUV);
    float4 grassColor1 = float4(0.32f, 0.67f, 0.24f, 1.0f);
    float4 grassColor2 = float4(0.7f, 1.0f, 0.19f, 1.0f);
    float4 finalColor = lerp(grassColor1, grassColor2, (1.0f * noiseTex));

    // ���C�e�B���O�̉e���𑐂̐F�ɉ��Z
    finalColor *= diffuse;

    return finalColor;
}
