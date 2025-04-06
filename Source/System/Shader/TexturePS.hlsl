#define BIAS 0.0006f        //�V���M�[�}���̃o�C�A�X�l
#define SHADOWSIZE 8192.0f  //�V���h�E�}�b�v�̃T�C�Y

SamplerState texSampler : register(s0);                 //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1);    //�e�p�T���v���[
Texture2D<float4> _MainTex : register(t0);              //�x�[�X�e�N�X�`��
Texture2D<float4> _NormalMap : register(t1);            //�m�[�}���}�b�v
Texture2D<float> shadowMap : register(t2);              //�V���h�E�}�b�v

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
    float4 svpos : SV_POSITION;         //���W
    float3 normal : NORMAL;             //�m�[�}��
    float2 uv : TEXCOORD0;              //UV
    float4 shadowPos : TEXCOORD1;       //�e�̈ʒu
    float4 tanLightDir : TEXCOORD2;     //�]�@��
    float3 tanHalfWayVec : TEXCOORD3;   //�ڐ�
};

float ShadowCalculation(float4 shadowPos)
{
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    //�e�̏_�炩�������肷�邽�߂̕�����(�T���v�����O��)
    int numSamples = 4; //�T���v���� (�����قǊ��炩�����d��)
    float total = 0.0f;
    float2 texelSize = float2(1.0f / SHADOWSIZE, 1.0f / SHADOWSIZE);
    
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
    //�e�N�X�`��
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //�@���}�b�v
    float2 tiledUV = input.uv * float2(5.0f, 10.0f);
    float4 normalTexColor = _NormalMap.Sample(texSampler, tiledUV);
    
    //���C�g�̕������t��
    float3 lightVec = lightDir.xyz;
    float lightIntensity = saturate(dot(input.normal, -lightVec));
    //float lightIntensity = 0.8f;
    
    //�V���h�E�̌v�Z
    float shadowFactor = ShadowCalculation(input.shadowPos);
    //float shadowFactor = 1.0f;
    
    float4 shadowPower = shadowFactor * lightIntensity;

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float4 lighting = lerp(ambientColor, diffuseColor, shadowPower);

    //���C�e�B���O���ʂƃe�N�X�`���J���[���|�����킹��
    float4 finalColor = lighting * tex;
    
    //float3 E = normalize(input.eyePos); // �����x�N�g��

    float3 N = 2.0f * normalTexColor.xyz - 1.0; // �@���}�b�v����̖@��
    N *= 0.15f;

    float3 blendedNormal = normalize(lerp(input.normal, N, saturate(0.5f)));

    //float3 R = reflect(-E, N); // ���˃x�N�g��
    //float amb = input.tanLightDir.w; // �����̋���
    
    //finalColor *= max(0, dot(blendedNormal, -lightVec));
    //finalColor += 0.3f * pow(max(0, dot(R, L)), 8);
    
    finalColor.a = cameraEyePos.w;

    return finalColor;
}
