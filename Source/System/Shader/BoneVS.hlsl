cbuffer ModelConstantBuffer : register(b0)
{
	matrix modelMatrix;			//���f���}�g���b�N�X
	matrix viewMatrix;			//�r���[�}�g���b�N�X
	matrix projectionMatrix;	//�v���W�F�N�V�����}�g���b�N�X
    matrix lightViewProjMatrix;	//�f�B���N�V���i�����C�g�̏��
    matrix normalMatrix;		//���f���̃X�P�[���A��]�Ȃǂ�input.normal�ɂ��K������p
    float4 eyePos;              //�J�����̈ʒu
    float4 lightPos;            //���C�g�̈ʒu
}

cbuffer BoneMatrices : register(b1)
{
	matrix boneMatrices[512];	    //�{�[���}�g���b�N�X�i�ő�512��)
}

cbuffer Constants : register(b2)	//����������1�x�����������s���Ȃ�����
{
    uint vertexCount;				//���_��
    uint shapeCount;				//�V�F�C�v�L�[�̐�
};

Texture2D<float4> ShapeDeltasTexture : register(t0);        //�V�F�C�v�L�[���Ƃ̈ʒu�ψʃf�[�^
StructuredBuffer<float> ShapeWeights : register(t1);		//�e�V�F�C�v�L�[�̃E�F�C�g

float4x4 InvTangentMatrix(float3 tangent, float3 binormal, float3 normal);

struct VSInput
{
	float3 pos : POSITION;				//���_���W
    float4 boneWeights : BONEWEIGHTS;	//�e���_�̃{�[���̉e���x
    uint4 boneIDs : BONEIDS;			//�e���_�ɉe����^����{�[���̃C���f�b�N�X
	uint vertexID : VERTEXID;			//���_��ID
    float3 normal : NORMAL;             //�@��
	float2 uv : TEXCOORD;				//UV
    float3 tangent : TANGENT;           //�ڐ�
    float3 binormal : BINORMAL;         //�]�@��
};

struct VSOutput
{
	float4 svpos : SV_POSITION;         //���W
    float3 normal : NORMAL;		        //�m�[�}���}�b�v
	float2 uv : TEXCOORD0;		        //UV
    float4 shadowPos : TEXCOORD1;       //�Z���t�V���h�E�̓��e�ʒu
    float4 tanLightDir : TEXCOORD2;     //�ڐ�
    float3 tanHalfWayVec : TEXCOORD3;   //�]�@��
};

//���_ID�ƃV�F�C�v�L�[��ID���瑊�Έʒu���擾
float3 GetShapeDelta(uint vertexID, uint shapeID)
{
    //�E�F�C�g�����ɏ������ꍇ�͖���
    float weight = ShapeWeights[shapeID];
    if (weight < 0.001f)
        return float3(0.0f, 0.0f, 0.0f);
    
    //�V�F�C�v�L�[�̃e�N�X�`���T�C�Y���擾
    uint width = 0, height = 0;
    ShapeDeltasTexture.GetDimensions(width, height);

    //���W
    uint x = vertexID % width;
    uint y = (vertexID / width) * shapeCount + shapeID;

    return ShapeDeltasTexture.Load(int3(x, y, 0)).xyz * weight;
}

VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	
	//�{�[���𔽉f
    matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

	//�V�F�C�v�L�[�̉e�������Z
    float3 shapePos = input.pos;
    for (uint i = 0; i < shapeCount; i++)
    {
        shapePos += GetShapeDelta(input.vertexID, i);
    }
	
    float4 localPos = mul(float4(shapePos, 1.0f), skinMatrix);	//���_���W
	float4 worldPos = mul(modelMatrix, localPos);				//���[���h���W�ɕϊ�
	float4 viewPos = mul(viewMatrix, worldPos);					//�r���[���W�ɕϊ�
	float4 projPos = mul(projectionMatrix, viewPos);			//���e�ϊ�

	//�{�[���A�j���[�V�����ɂ��@���ϊ�
    float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);
    skinnedNormal += input.boneWeights.x * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.x]);
    skinnedNormal += input.boneWeights.y * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.y]);
    skinnedNormal += input.boneWeights.z * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.z]);
    skinnedNormal += input.boneWeights.w * mul(input.normal, (float3x3)boneMatrices[input.boneIDs.w]);
    
    float3 nor = normalize(mul(input.normal, (float3x3)modelMatrix));
    float3 bi = normalize(mul(input.binormal, (float3x3)modelMatrix));
    float3 tan = normalize(mul(input.tangent, (float3x3)modelMatrix));
    //float3 nor = normalize(input.normal);
    //float3 bi = normalize(input.binormal);
    //float3 tan = normalize(input.tangent);

    float4x4 invMat = InvTangentMatrix(tan, bi, nor);

    //output.tanLightDir = mul(lightViewProjMatrix._41_42_43_44, invMat).xyz;
    output.tanLightDir = mul(lightPos, invMat);
    
    float4 halfWayVec = normalize(normalize(eyePos - projPos) + lightPos);
    output.tanHalfWayVec = mul(halfWayVec, invMat).xyz;
	
	output.svpos = projPos;			//���e�ϊ����ꂽ���W
    output.normal = normalize(mul(normalMatrix, float4(skinnedNormal, 1.0f)).xyz);
	output.uv = input.uv;			//UV
    output.shadowPos = mul(lightViewProjMatrix, worldPos);

	return output;				//�s�N�Z���V�F�[�_�[�ɓn��
}


float4x4 InvTangentMatrix(float3 tangent, float3 binormal, float3 normal)
{
    float4x4 mat =
    {
        float4(tangent, 0.0f),
        float4(binormal, 0.0f),
        float4(normal, 0.0f),
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    return transpose(mat); //�]�u
}