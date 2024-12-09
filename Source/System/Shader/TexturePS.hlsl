#define BIAS 0.005f     //�V���M�[�}���̃o�C�A�X�l

SamplerState texSampler : register(s0); //�e�N�X�`���p�T���v���[
SamplerComparisonState shadowSampler : register(s1); //�e�N�X�`���p�T���v���[
Texture2D<float4> _MainTex : register(t0); //�e�N�X�`��
Texture2D<float> shadowMap : register(t1); //�V���h�E�}�b�v

//�萔�o�b�t�@
cbuffer LightBuffer : register(b0)
{
    float3 lightDir; //���C�g�̕���
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
    float shadowFactor = 1.0f;
    
    // �V���h�E�}�b�v���W�ϊ�
    float3 posFromLightVP = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    //�V���h�E�̗L���𔻒�
    shadowFactor = shadowMap.SampleCmp(shadowSampler, shadowUV, posFromLightVP.z - BIAS);

    return shadowFactor;
}

float4 pixel(VSOutput input) : SV_TARGET
{
    //�e�N�X�`��
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //�@���𐳋K��
    float3 normal = normalize(input.normal);

    //���C�g�̕������t��
    float lightIntensity = saturate(dot(normal, -lightDir));

    //�V���h�E�̌v�Z
    float shadowFactor = ShadowCalculation(input.shadowPos);
    
    float3 diffuse = float3(1.0f, 1.0f, 1.0f) * lightIntensity;

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float3 lighting = ambientColor + shadowFactor * diffuse;

    //���C�e�B���O���ʂƃe�N�X�`���J���[���|�����킹��
    float4 finalColor = float4(lighting, 1.0f) * tex;

    return finalColor;
}
