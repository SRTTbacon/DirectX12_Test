cbuffer ModelConstantBuffer : register(b0)
{
    matrix modelMatrix;         //���f���}�g���b�N�X
    matrix viewMatrix;          //�r���[�}�g���b�N�X
    matrix projectionMatrix;    //�v���W�F�N�V�����}�g���b�N�X
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
    float4 color : COLOR;       //�F
};

VSOutput vert(VSInput input)
{
    VSOutput output = (VSOutput) 0;
	
	float4 localPos = float4(input.pos, 1.0f);		    //���_���W
    float4 worldPos = mul(modelMatrix, localPos);       //���[���h���W�ɕϊ�
    float4 viewPos = mul(viewMatrix, worldPos);         //�r���[���W�ɕϊ�
    float4 projPos = mul(projectionMatrix, viewPos);    //���e�ϊ�

    output.svpos = projPos;         //���e�ϊ����ꂽ���W
    output.color = input.color;     //���_�F
    return output;                  //�s�N�Z���V�F�[�_�[�ɓn��
}
