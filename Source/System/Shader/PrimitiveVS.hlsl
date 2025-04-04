cbuffer ModelConstantBuffer : register(b0)
{
    matrix modelMatrix;         //���f���}�g���b�N�X
    matrix viewMatrix;          //�r���[�}�g���b�N�X
    matrix projectionMatrix;    //�v���W�F�N�V�����}�g���b�N�X
    matrix lightViewProjMatrix; //�f�B���N�V���i�����C�g�̏��
    float4 eyePos;              //�J�����̈ʒu
    float4 lightPos;            //���C�g�̈ʒu
}

cbuffer MeshBuffer : register(b1)
{
    matrix meshMatrix;          //���b�V���P�ʂ̃}�g���b�N�X
    matrix normalMatrix;        //���f���̃X�P�[���A��]�Ȃǂ�input.normal�ɂ��K������p
    float time;             //���ԏ��i�V�F�[�_�[�œ��I�ɍX�V�j
}

struct VSInput
{
    float3 pos : POSITION;              //���_���W
    float4 boneWeights : BONEWEIGHTS;   //�e���_�̃{�[���̉e���x
    uint4 boneIDs : BONEIDS;            //�e���_�ɉe����^����{�[���̃C���f�b�N�X (�{�[�����Ȃ����f���͂��ׂ�0)
    uint vertexID : VERTEXID;           //���_��ID
    float3 normal : NORMAL;             //�@��
    float2 uv : TEXCOORD;               //UV
    float3 tangent : TANGENT;           //�ڐ�
    float3 binormal : BINORMAL;         //�]�@��
};

struct VSOutput
{
    float4 svpos : SV_POSITION;     //�X�N���[�����W
    float3 normal : NORMAL;         //�@��
    float2 uv : TEXCOORD;           //UV
    float4 shadowPos : TEXCOORD1;   //�e�̈ʒu
    float3 worldPos : TEXCOORD2;    //���[���h���W
};

VSOutput vert(VSInput input)
{
    VSOutput output = (VSOutput)0;
    matrix temp = meshMatrix;
	float4 localPos = float4(input.pos, 1.0f);		    //���_���W
    float4 meshPos = mul(temp, localPos);               //���[�J�����W�ɕϊ�
    float4 worldPos = mul(modelMatrix, meshPos);        //���[���h���W�ɕϊ�
    float4 viewPos = mul(viewMatrix, worldPos);         //�r���[���W�ɕϊ�
    float4 projPos = mul(projectionMatrix, viewPos);    //���e�ϊ�

    output.svpos = projPos;         //���e�ϊ����ꂽ���W
    output.worldPos = worldPos.xyz; //���[���h���W
    output.normal = normalize(mul(normalMatrix, float4(input.normal, 0.0f)).xyz);
    output.uv = input.uv;           //UV
    output.shadowPos = mul(lightViewProjMatrix, worldPos);
    return output; //�s�N�Z���V�F�[�_�[�ɓn��
}
