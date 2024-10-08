cbuffer ModelConstantBuffer : register(b0)
{
	matrix modelMatrix;       //���f���}�g���b�N�X
	matrix viewMatrix;        //�r���[�}�g���b�N�X
	matrix projectionMatrix;  //�v���W�F�N�V�����}�g���b�N�X
}

cbuffer BoneMatrices : register(b1)
{
	matrix boneMatrices[512];	//�{�[���}�g���b�N�X�i�ő�512��)
}

cbuffer ShapeWeights : register(b2)
{
    float shapeWeight[512];		//�e�V�F�C�v�L�[�̃E�F�C�g (�ő�512��)
}

struct VSInput
{
	float3 pos : POSITION;				//���_���W
	float3 normal : NORMAL;				//�@��
	float2 uv : TEXCOORD;				//UV
	float4 color : COLOR;				//���_�F
	float4 boneWeights : BONEWEIGHTS;	//�e���_�̃{�[���̉e���x
	uint4 boneIDs : BONEIDS;			//�e���_�ɉe����^����{�[���̃C���f�b�N�X
    float3 shapePos : SHAPEPOSITION;	//�V�F�C�v�L�[�̓K����̈ʒu
    uint shapeID : SHAPEID;				//�Ή�����V�F�C�v�L�[�̃C���f�b�N�X
};

struct VSOutput
{
	float4 svpos : SV_POSITION; //���W
	float4 color : COLOR;		//�F
	float2 uv : TEXCOORD;		//UV
};

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	
	//�{�[���𔽉f
	matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

    float3 shapePos = input.pos + input.shapePos * shapeWeight[input.shapeID];
	
    float4 localPos = mul(float4(shapePos, 1.0f), skinMatrix); //���_���W
	//float4 localPos = float4(input.pos, 1.0f);				//���_���W
	float4 worldPos = mul(modelMatrix, localPos);				//���[���h���W�ɕϊ�
	float4 viewPos = mul(viewMatrix, worldPos);					//�r���[���W�ɕϊ�
	float4 projPos = mul(projectionMatrix, viewPos);			//���e�ϊ�

	output.svpos = projPos;		//���e�ϊ����ꂽ���W
	output.color = input.color; //���_�F
	output.uv = input.uv;		//UV
	return output;				//�s�N�Z���V�F�[�_�[�ɓn��
}
