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

cbuffer Constants : register(b2) //初期化時に1度だけしか実行しないもの
{
    uint vertexCount; //頂点数
    uint shapeCount; //シェイプキーの数
};

struct VSInput
{
    float3 position : POSITION;         //頂点座標
    float4 boneWeights : BONEWEIGHTS;   //各頂点のボーンの影響度 (ボーンが存在しない場合はすべて0)
    uint4 boneIDs : BONEIDS;            //各頂点に影響を与えるボーンのインデックス (ボーンが存在しない場合はすべて0)
    uint vertexID : VERTEXID;           //頂点のID
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

Texture2D<float4> ShapeDeltasTexture : register(t0); //シェイプキーごとの位置変位データ
StructuredBuffer<float> ShapeWeights : register(t1); //各シェイプキーのウェイト

//頂点IDとシェイプキーのIDから相対位置を取得
float3 GetShapeDelta(uint vertexID, uint shapeID)
{
    //ウェイトが非常に小さい場合は無視
    float weight = ShapeWeights[shapeID];
    if (weight < 0.001f)
        return float3(0.0f, 0.0f, 0.0f);
    
    //シェイプキーのテクスチャサイズを取得
    uint width = 0, height = 0;
    ShapeDeltasTexture.GetDimensions(width, height);

    //座標
    uint x = vertexID % width;
    uint y = (vertexID / width) * shapeCount + shapeID;

    return ShapeDeltasTexture.Load(int3(x, y, 0)).xyz * weight;
}

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;

    //シェイプキーを適応(存在すれば)
    float3 shapePos = input.position;
    for (uint i = 0; i < shapeCount; i++)
    {
        shapePos += GetShapeDelta(input.vertexID, i);
    }

    float4 localPos = float4(0.0f, 0.0f, 0.0f, 0.0f);

    //ボーンを適応(存在すれば)
    if (input.boneWeights.x == 0.0f && input.boneWeights.y == 0.0f && input.boneWeights.z == 0.0f && input.boneWeights.w == 0.0f)
    {
        localPos = float4(shapePos, 1.0f); //頂点座標
    }
    else
    {
        //ボーンを反映
        matrix skinMatrix = input.boneWeights.x * boneMatrices[input.boneIDs.x] + input.boneWeights.y * boneMatrices[input.boneIDs.y] +
		input.boneWeights.z * boneMatrices[input.boneIDs.z] + input.boneWeights.w * boneMatrices[input.boneIDs.w];

        localPos = mul(float4(shapePos, 1.0f), skinMatrix); //頂点座標
    }
    float4 worldPos = mul(modelMatrix, localPos);       //ワールド座標に変換
    output.position = mul(lightViewProjMatrix, worldPos);
    return output;
}