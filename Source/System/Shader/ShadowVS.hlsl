//影のみのシェーダー

//バッファを使いまわすためBoneVSと同じ内容
cbuffer TransformBuffer : register(b0)
{
    matrix modelMatrix; //モデルマトリックス
    matrix viewMatrix; //ビューマトリックス
    matrix projectionMatrix; //プロジェクションマトリックス
    matrix lightViewProjMatrix; //ディレクショナルライトの情報
    matrix normalMatrix; //モデルのスケール、回転などをinput.normalにも適応する用
    float4 eyePos;              //カメラの位置
};

cbuffer BoneMatrices : register(b1)
{
    matrix boneMatrices[512]; //ボーンマトリックス（最大512個)
}

struct VSInput
{
    float3 position : POSITION;         //頂点座標
    float4 boneWeights : BONEWEIGHTS;   //各頂点のボーンの影響度 (ボーンが存在しない場合はすべて0)
    uint4 boneIDs : BONEIDS;            //各頂点に影響を与えるボーンのインデックス (ボーンが存在しない場合はすべて0)
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    //ボーンを反映
    matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];
    
    float4 localPos = mul(float4(input.position, 1.0f), skinMatrix); //頂点座標
    float4 worldPos = mul(modelMatrix, localPos);       //ワールド座標に変換
    output.position = mul(lightViewProjMatrix, worldPos);
    return output;
}