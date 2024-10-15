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

cbuffer Constants : register(b2)	//����������1�x�����������s���Ȃ�����
{
    uint vertexCount;				//���_��
    uint shapeCount;				//�V�F�C�v�L�[�̐�
};

StructuredBuffer<float3> ShapeDeltasTexture : register(t1);		//�V�F�C�v�L�[���Ƃ̈ʒu�ψʃf�[�^
StructuredBuffer<float> ShapeWeights : register(t2);			//�e�V�F�C�v�L�[�̃E�F�C�g

struct VSInput
{
	float3 pos : POSITION;				//���_���W
	float3 normal : NORMAL;				//�@��
	float2 uv : TEXCOORD;				//UV
	float4 color : COLOR;				//���_�F
	float4 boneWeights : BONEWEIGHTS;	//�e���_�̃{�[���̉e���x
	uint4 boneIDs : BONEIDS;			//�e���_�ɉe����^����{�[���̃C���f�b�N�X
	uint vertexID : VERTEXID;			//���_��ID
};

struct VSOutput
{
	float4 svpos : SV_POSITION; //���W
	float4 color : COLOR;		//�F
	float2 uv : TEXCOORD;		//UV
};

//���_ID�ƃV�F�C�v�L�[��ID���瑊�Έʒu���擾
float3 GetShapeDelta(uint vertexID, uint shapeID)
{
    return ShapeDeltasTexture[vertexID + shapeID * vertexCount] * ShapeWeights[shapeID];
}

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	
	//�{�[���𔽉f
	matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

    float3 shapePos = input.pos;

	// �V�F�C�v�L�[�̉e�������Z
    for (uint i = 0; i < shapeCount; i++)
    {
        shapePos += GetShapeDelta(input.vertexID, i);
    }
	
    float4 localPos = mul(float4(shapePos, 1.0f), skinMatrix);	//���_���W
	float4 worldPos = mul(modelMatrix, localPos);				//���[���h���W�ɕϊ�
	float4 viewPos = mul(viewMatrix, worldPos);					//�r���[���W�ɕϊ�
	float4 projPos = mul(projectionMatrix, viewPos);			//���e�ϊ�

	output.svpos = projPos;		//���e�ϊ����ꂽ���W
	output.color = input.color; //���_�F
	output.uv = input.uv;		//UV
	return output;				//�s�N�Z���V�F�[�_�[�ɓn��
}
