#define BIAS 0.005f     //�V���M�[�}���̃o�C�A�X�l

struct VSOutput
{
	float4 svpos : SV_POSITION;         //���_�V�F�[�_�[���痈�����W
    float3 normal : NORMAL;             //���_�V�F�[�_�[����k�m�[�}���}�b�v
	float4 color : COLOR;		        //���_�V�F�[�_�[���痈���F
	float2 uv : TEXCOORD;		        //���_�V�F�[�_�[���痈��UV
    float4 lightSpacePos : TEXCOORD0;   //�f�B���N�V���i�����C�g�̏��
    float3 lightDir : TEXCOORD1;        //�f�B���N�V���i�����C�g�̕���
};

SamplerState texSampler : register(s0);	    //�e�N�X�`���p�T���v���[
Texture2D _MainTex : register(t0);	        //�e�N�X�`��

SamplerState shadowSampler : register(s1);  //�V���h�E�}�b�v�p�T���v���[
Texture2D shadowMap : register(t1);         //�V���h�E�}�b�v

float ShadowCalculation(float4 lightSpacePos)
{
    //�V���h�E�}�b�v�̃e�N�X�`�����W�ɕϊ�
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5f + 0.5f;

    //�V���h�E�}�b�v�Ő[�x���T���v��
    float shadowMapDepth = shadowMap.Sample(shadowSampler, projCoords.xy).r;
    
    //���݂̃s�N�Z���̐[�x�ƃV���h�E�}�b�v�̐[�x���r
    float currentDepth = projCoords.z;
    return currentDepth > shadowMapDepth + BIAS ? 0.0f : 1.0f; // �e���ǂ����𔻒�
}

float4 pixel(VSOutput input) : SV_TARGET
{
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    // �V���h�E����
    float shadow = ShadowCalculation(input.lightSpacePos);
    
    // ���C�g�̉e�����v�Z
    float3 light = max(dot(normalize(input.normal), -input.lightDir), 0.0f);

    return tex * float4(light, 1.0f) * shadow;
}
