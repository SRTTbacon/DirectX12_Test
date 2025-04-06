//#define BIAS 0.0006f        //�V���M�[�}���̃o�C�A�X�l
#define SHADOWSIZE 8192.0f  //�V���h�E�}�b�v�̃T�C�Y

SamplerState texSampler : register(s0); //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1); //�e�p�T���v���[
Texture2D<float4> _MainTex : register(t0); //�x�[�X�e�N�X�`��
Texture2D<float4> _NormalMap : register(t1); //�m�[�}���}�b�v
Texture2D<float> shadowMap : register(t2); //�V���h�E�}�b�v

//�萔�o�b�t�@
cbuffer PixelBuffer : register(b0)
{
    float4 lightDir;        //���C�g�̕���
    float4 ambientColor;    //�e�̐F
    float4 diffuseColor;    //�W���̐F
    float4 cameraEyePos;    //�J�����̈ʒu
    float4 fogColor;        //�t�H�O�̐F
    float2 fogStartEnd;     //�t�H�O�̊J�n�����A�I������
};

struct VSOutput
{
    float4 svpos : SV_POSITION; //���W
    float3 normal : NORMAL; //�@��
    float2 uv : TEXCOORD0; //UV
    float4 shadowPos : TEXCOORD1; //�e�̈ʒu
    float4 tanLightDir : TEXCOORD2; //�]�@��
    float3 tanHalfWayVec : TEXCOORD3; //�ڐ�
};

//�@���ƌ��̕����ɉ������o�C�A�X
float ComputeShadowBias(float3 normal, float3 lightDir)
{
    float cosTheta = saturate(dot(normal, lightDir));
    return max(0.0015 * (1.0 - cosTheta), 0.0005f); //�@�������ƕ��s�Ȃقǃo�C�A�X�����炷
}

//Toon���̒i�K�I�Ȍ�
float ToonShading(float intensity)
{
    if (intensity > 0.5f)
        return 1.0f;
    else
        return 0.2f;
}

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

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
    for (int i = 0; i < 8; ++i)
    {
        float2 sampleOffset = poissonDisk[i] * texelSize;
        float2 shadowSampleUV = shadowUV + sampleOffset;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }

    return total / 8.0f; // ���ω�
}

float4 main(VSOutput input) : SV_TARGET
{
    //�e�N�X�`��
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //�@���}�b�v
    float2 tiledUV = input.uv * float2(5.0f, 10.0f);
    float4 normalTexColor = _NormalMap.Sample(texSampler, tiledUV);
    
    //���C�g�̕������t��
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));
    //lightIntensity = smoothstep(0.49, 0.51, lightIntensity);
    lightIntensity = ToonShading(lightIntensity);
    
    //�V���h�E�̌v�Z
    float bias = ComputeShadowBias(input.normal, lightVec);
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    shadowFactor = smoothstep(0.35, 0.85, shadowFactor);
    
    float shadowPower = lightIntensity * shadowFactor;
    shadowPower = max(0.5f, shadowPower);

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float4 lighting = lerp(ambientColor, diffuseColor, shadowPower);

    //���C�e�B���O���ʂƃe�N�X�`���J���[���|�����킹��
    float4 finalColor = lighting * tex;
    
    return finalColor;
}
