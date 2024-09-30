cbuffer ModelConstantBuffer : register(b0) {
	matrix modelMatrix;       // ���f���}�g���b�N�X
	matrix viewMatrix;        // �r���[�}�g���b�N�X
	matrix projectionMatrix;  // �v���W�F�N�V�����}�g���b�N�X
}

cbuffer BoneMatrices : register(b1) {
	matrix boneMatrices[512]; // �{�[���}�g���b�N�X�i�ő�512�̃{�[�����T�|�[�g�j
}

struct VSInput
{
	float3 pos : POSITION; // ���_���W
	float3 normal : NORMAL; // �@��
	float2 uv : TEXCOORD; // UV
	float4 color : COLOR; // ���_�F
	float4 boneWeights : BONEWEIGHTS;  // �e���_�̃{�[���E�F�C�g
	uint4 boneIDs : BONEIDS;       // �e���_�ɉe����^����{�[��ID
};

struct VSOutput
{
	float4 svpos : SV_POSITION; // �ϊ����ꂽ���W
	float4 color : COLOR; // �ϊ����ꂽ�F
	float2 uv : TEXCOORD;
};

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0; // �A�E�g�v�b�g�\���̂��`����
	
	matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

	float4 localPos = mul(float4(input.pos, 1.0f), skinMatrix); // ���_���W
	//float4 localPos = float4(input.pos, 1.0f); // ���_���W
	float4 worldPos = mul(modelMatrix, localPos); // ���[���h���W�ɕϊ�
	float4 viewPos = mul(viewMatrix, worldPos); // �r���[���W�ɕϊ�
	float4 projPos = mul(projectionMatrix, viewPos); // ���e�ϊ�

	output.svpos = projPos; // ���e�ϊ����ꂽ���W���s�N�Z���V�F�[�_�[�ɓn��
	output.color = input.color; // ���_�F�����̂܂܃s�N�Z���V�F�[�_�[�ɓn��
	output.uv = input.uv;
	return output;
}
