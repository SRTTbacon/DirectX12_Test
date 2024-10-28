#define BIAS 0.005f     //�V���M�[�}���̃o�C�A�X�l

//�萔�o�b�t�@
cbuffer LightBuffer : register(b0)
{
    float3 lightDir;        //���C�g�̕���
    float4 ambientColor;    //�����̐F
    float4 diffuseColor;    //�g�U���̐F
    float4 specularColor;   //�X�y�L�������̐F
};

struct VSOutput
{
	float4 svpos : SV_POSITION;         //���_�V�F�[�_�[���痈�����W
    float3 normal : NORMAL;             //���_�V�F�[�_�[����k�m�[�}���}�b�v
	float4 color : COLOR;		        //���_�V�F�[�_�[���痈���F
	float2 uv : TEXCOORD;		        //���_�V�F�[�_�[���痈��UV
    float4 lightSpacePos : TEXCOORD1;   //�f�B���N�V���i�����C�g�̏��
};

SamplerState texSampler : register(s0);	    //�e�N�X�`���p�T���v���[
Texture2D<float4> _MainTex : register(t0);	        //�e�N�X�`��

SamplerComparisonState shadowSampler : register(s1);  //�V���h�E�}�b�v�p�T���v���[
Texture2D shadowMap : register(t1);         //�V���h�E�}�b�v

float4 pixel(VSOutput input) : SV_TARGET
{
    float4 tex = _MainTex.Sample(texSampler, input.uv);

    //�@���𐳋K��
    float3 normal = normalize(input.normal);

    //���C�g�̕������t��
    float3 lightDirection = normalize(-lightDir);

    //�g�U���̌v�Z
    float NdotL = max(dot(normal, lightDirection), 0.0f);
    float3 diffuse = diffuseColor.rgb * NdotL;

    //�����̌v�Z
    float3 ambient = ambientColor.rgb;

    //�V���h�E�}�b�v�̃T���v�����O�Ɖe���� (�y��PCF 2x2)
    float shadowFactor = 0.0f;
    float2 texelSize = 1.0f / 2048.0f;

    for (int x = -1; x <= 1; x += 2)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            shadowFactor += shadowMap.SampleCmpLevelZero(shadowSampler, input.lightSpacePos.xy + float2(x, y) * texelSize, input.lightSpacePos.z);
        }
    }
    //shadowFactor *= 0.25f;
    shadowFactor += 1.25f;

    //�V���h�E���������Ă���Ό������������� (0.0f �Ȃ犮�S�ȉe�A1.0f �Ȃ�e�Ȃ�)
    float3 lighting = ambient + shadowFactor * diffuse;

    //���C�e�B���O���ʂƃe�N�X�`���J���[���|�����킹��
    float4 finalColor = float4(lighting, 1.0f) * tex;

    return finalColor;
}
