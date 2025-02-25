struct PSInput
{
    float4 position : SV_POSITION;
    float alpha : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(input.position.x, input.position.y, input.position.z, input.alpha);
}