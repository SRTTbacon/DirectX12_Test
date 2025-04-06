#define BIAS 0.001f     //�V���M�[�}���̃o�C�A�X�l
#define SHADOWSIZE 8192.0f

SamplerState texSampler : register(s0);         //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1); //�e�N�X�`���p�T���v���[
Texture2D<float4> _MainTex : register(t0);      //�e�N�X�`��
Texture2D<float4> _NormalMap : register(t1);    //�m�[�}���}�b�v
Texture2D<float> shadowMap : register(t2);      //�V���h�E�}�b�v

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

struct VSOutput
{
    float4 svpos : SV_POSITION;     //�������W
    float3 normal : NORMAL; //�m�[�}���}�b�v
    float2 uv : TEXCOORD;           //����UV
    float4 shadowPos : TEXCOORD1;   //�e�̈ʒu
    float3 worldPos : TEXCOORD2;    //���[���h���W
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

float2 Rotate(float2 v, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float ShadowCalculation(float4 shadowPos, float bias)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5, -0.5);

    float isInside = step(0.0f, shadowUV.x) * step(shadowUV.x, 1.0f) *
                     step(0.0f, shadowUV.y) * step(shadowUV.y, 1.0f);

    float angle = rand(shadowUV * SHADOWSIZE) * 6.2831853f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    float total = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        float2 rotated = Rotate(poissonDisk[i], angle);
        float2 shadowSampleUV = shadowUV + rotated * texelSize * 1.5f;
        float sampleDepth = shadowMap.SampleCmpLevelZero(shadowSampler, shadowSampleUV, posFromLightVP.z - bias);
        total += sampleDepth;
    }

    float shadowFactor = total / 8.0f;
    shadowFactor = lerp(1.0f, shadowFactor, isInside);
    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //�e�N�X�`��
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //���C�g�̕������t��
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));

    float bias = ComputeShadowBias(input.normal, lightVec);

    //�V���h�E�̌v�Z
    float shadowFactor = ShadowCalculation(input.shadowPos, bias);
    
    float shadowPower = lightIntensity * shadowFactor;
    shadowPower = max(0.1f, shadowPower);

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float4 diffuse = diffuseColor * shadowPower;

    float4 lighting = lerp(ambientColor, diffuseColor, shadowPower);
    
    lighting *= tex;
    
    //�J��������̋����v�Z
    float distance = abs(length(input.worldPos - cameraEyePos.xyz));

    //0 �` 1�Ƀ}�b�s���O
    float fogFactor = saturate((distance - fogStartEnd.x) / (fogStartEnd.y - fogStartEnd.x));
    fogFactor = min(0.9f, fogFactor);
    
    lighting = lerp(lighting, fogColor, fogFactor);

    lighting.a = cameraEyePos.w;

    return lighting;
}