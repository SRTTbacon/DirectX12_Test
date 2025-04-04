cbuffer ViewProjection : register(b0)
{
    matrix viewProj;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD;
};

struct VSInput
{
    float3 position : POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    //���_�ʒu�����̂܂܃X�J�C�{�b�N�X�̃e�N�X�`�����W�Ɏg�p
    output.texCoord = input.position;

    //�J�����̈ʒu���l�����A�X�J�C�{�b�N�X�̉����ɔz�u
    float4 pos = float4(input.position, 0.0f);
    pos = mul(viewProj, pos);
    //�����ɔz�u���邽�߁AW���������̂܂�X,Y,Z�ɓK�p
    output.position = float4(pos.x, pos.y, pos.w, pos.w);
    
    return output;
}