//カメラ情報
cbuffer Camera : register(b0)
{
    float3 cameraPosition;
    float3 cameraForward;
    float3 cameraRight;
    float3 cameraUp;
};

//光源情報
cbuffer DirectionalLight : register(b1)
{
    float3 lightDirection;
    float3 lightColor;
};

//ボーン情報
cbuffer Bones : register(b2)
{
    row_major float4x4 boneMatrices[512];
};

//マテリアル情報
struct Material
{
    float3 diffuseColor;
    float3 specularColor;
    float specularPower;
};

//属性（各ジオメトリから渡されるデータ）
struct Attributes
{
    float3 localPosition;
    float4 boneWeights;
    uint4 boneIndices;
    float3 normal;
    float2 uv;
    uint vertexID;
};

//ペイロード（レイの結果を保持する構造体）
struct RayPayload
{
    float3 color;
};

//UV座標をスクリーン空間から計算
float2 ComputeScreenUV(uint2 pixelIndex, uint2 screenDimensions)
{
    return (float2(pixelIndex) + 0.5) / float2(screenDimensions);
}

//ワールド座標におけるレイ方向を計算
float3 ComputeWorldRayDirection(float2 screenUV, float3 cameraPos, float3 forward, float3 right, float3 up)
{
    float3 rayDir = normalize(forward + (screenUV.x - 0.5) * 2.0 * right + (screenUV.y - 0.5) * -2.0 * up);
    return rayDir;
}
