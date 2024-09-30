#define MAX_BONES 4 // ボーンの最大数を定義

cbuffer Transform : register(b0)
{
	matrix World;
	matrix View;
	matrix Proj;
	matrix BoneTransforms[MAX_BONES]; // ボーントランスフォーム
}

struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	uint4 boneIndices : BLENDINDICES; // ボーンインデックス
	float4 weights : BLENDWEIGHT; // ボーンウェイト
};

struct VSOutput
{
	float4 position : SV_POSITION; // 最終クリッピング位置
	float3 normal : NORMAL;        // 法線
	float2 uv : TEXCOORD;   // テクスチャ座標
};

VSOutput vert(VSInput input)
{
    // ボーントランスフォームを計算
    float4 skinPosition = float4(0.0, 0.0, 0.0, 0.0);
    float4 inputPos = float4(input.pos, 1.0); // input.posをfloat4に変換

    for (int i = 0; i < 4; i++) { // ボーンインデックスは最大4つ
        int boneIndex = (int)input.boneIndices[i];
        float weight = input.weights[i];

        if (weight > 0.0) { // ウェイトがゼロより大きい場合
            skinPosition += mul(BoneTransforms[boneIndex], inputPos) * weight; // 変換を適用
        }
    }

    VSOutput output;

    float4 worldPos = mul(skinPosition, World);
    float4 viewPos = mul(worldPos, View);
    float4 projPos = mul(viewPos, Proj);

    output.position = projPos;
    output.normal = input.normal; // ここは必要に応じて処理を加えてください
    output.uv = input.uv; // テクスチャ座標を出力
    return output;
}