#define MAX_BONES 4 // �{�[���̍ő吔���`

cbuffer Transform : register(b0)
{
	matrix World;
	matrix View;
	matrix Proj;
	matrix BoneTransforms[MAX_BONES]; // �{�[���g�����X�t�H�[��
}

struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	uint4 boneIndices : BLENDINDICES; // �{�[���C���f�b�N�X
	float4 weights : BLENDWEIGHT; // �{�[���E�F�C�g
};

struct VSOutput
{
	float4 position : SV_POSITION; // �ŏI�N���b�s���O�ʒu
	float3 normal : NORMAL;        // �@��
	float2 uv : TEXCOORD;   // �e�N�X�`�����W
};

VSOutput vert(VSInput input)
{
    // �{�[���g�����X�t�H�[�����v�Z
    float4 skinPosition = float4(0.0, 0.0, 0.0, 0.0);
    float4 inputPos = float4(input.pos, 1.0); // input.pos��float4�ɕϊ�

    for (int i = 0; i < 4; i++) { // �{�[���C���f�b�N�X�͍ő�4��
        int boneIndex = (int)input.boneIndices[i];
        float weight = input.weights[i];

        if (weight > 0.0) { // �E�F�C�g���[�����傫���ꍇ
            skinPosition += mul(BoneTransforms[boneIndex], inputPos) * weight; // �ϊ���K�p
        }
    }

    VSOutput output;

    float4 worldPos = mul(skinPosition, World);
    float4 viewPos = mul(worldPos, View);
    float4 projPos = mul(viewPos, Proj);

    output.position = projPos;
    output.normal = input.normal; // �����͕K�v�ɉ����ď����������Ă�������
    output.uv = input.uv; // �e�N�X�`�����W���o��
    return output;
}