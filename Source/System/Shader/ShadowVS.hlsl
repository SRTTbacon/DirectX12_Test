//影のみのシェーダー

//バッファを使いまわすためBoneVSと同じ内容
cbuffer TransformBuffer : register(b0)
{
    matrix modelMatrix; //モデルマトリックス
    matrix viewMatrix; //ビューマトリックス
    matrix projectionMatrix; //プロジェクションマトリックス
    matrix lightViewProjMatrix; //ディレクショナルライトの情報
    matrix normalMatrix; //モデルのスケール、回転などをinput.normalにも適応する用
};

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), modelMatrix);
    output.position = mul(worldPosition, lightViewProjMatrix);
    return output;
}