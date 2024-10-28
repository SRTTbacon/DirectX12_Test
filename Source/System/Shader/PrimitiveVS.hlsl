cbuffer ModelConstantBuffer : register(b0)
{
    matrix modelMatrix;         //���f���}�g���b�N�X
    matrix viewMatrix;          //�r���[�}�g���b�N�X
    matrix projectionMatrix;    //�v���W�F�N�V�����}�g���b�N�X
    matrix lightViewProjMatrix; //�f�B���N�V���i�����C�g�̏��
    matrix normalMatrix; //���f���̃X�P�[���A��]�Ȃǂ�input.normal�ɂ��K������p
}

struct VSInput
{
    float3 pos : POSITION;  //���_���W
    float3 normal : NORMAL; // �@��
    float2 uv : TEXCOORD; // UV
    float4 color : COLOR;   //���_�F
};

struct VSOutput
{
    float4 svpos : SV_POSITION; //���W
    float3 normal : NORMAL; //�m�[�}���}�b�v
    float4 color : COLOR; //�F
    float2 uv : TEXCOORD; //UV
    float4 lightSpacePos : TEXCOORD1; //�f�B���N�V���i�����C�g
};

VSOutput vert(VSInput input)
{
    VSOutput output = (VSOutput)0;
	
	float4 localPos = float4(input.pos, 1.0f);		    //���_���W
    float4 worldPos = mul(modelMatrix, localPos);       //���[���h���W�ɕϊ�
    float4 viewPos = mul(viewMatrix, worldPos);         //�r���[���W�ɕϊ�
    float4 projPos = mul(projectionMatrix, viewPos);    //���e�ϊ�

    output.svpos = projPos; //���e�ϊ����ꂽ���W
    output.normal = mul(float4(input.normal, 1.0f), normalMatrix).xyz; //�m�[�}���}�b�v
    output.color = input.color; //���_�F
    output.uv = input.uv; //UV
    float4 lightSpacePos = mul(lightViewProjMatrix, worldPos);
    output.lightSpacePos = lightSpacePos / lightSpacePos.w; //���C�g��Ԃւ̕ϊ�
    return output; //�s�N�Z���V�F�[�_�[�ɓn��
}
