Texture2D screenTexture : register(t0);
SamplerState screenSampler : register(s0);

cbuffer NoiseSettings : register(b0)
{
    float time;
    float glitchIntensity; // �O���b�`�̋���
    float distortionStrength; // UV�̘c��
    float blockSize; // �u���b�N�m�C�Y�̃T�C�Y
    float noiseStrength; // �ʏ�m�C�Y�̋���
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

// �^����������
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// UV���������ɂ��炷
float2 ApplyUVGlitch(float2 uv)
{
    float glitchLine = step(0.5, random(float2(uv.y, time * 0.1))) * distortionStrength;
    uv.x += glitchLine * (random(uv) - 0.5) * 0.02;
    return clamp(uv, 0.0, 1.0);
}

// RGB���� (Chromatic Aberration)
float3 ApplyChromaticAberration(float2 uv)
{
    float2 redUV = uv + float2(0.002 * sin(time), 0.0);
    float2 blueUV = uv + float2(-0.002 * sin(time), 0.0);
    
    float3 color;
    color.r = screenTexture.Sample(screenSampler, redUV).r;
    color.g = screenTexture.Sample(screenSampler, uv).g;
    color.b = screenTexture.Sample(screenSampler, blueUV).b;

    return color;
}

// �����_���u���b�N�m�C�Y
float ApplyBlockNoise(float2 uv)
{
    float2 blockUV = floor(uv * blockSize) / blockSize;
    float noise = step(0.9, random(blockUV + time * 0.5));
    return noise;
}

// �ʏ�m�C�Y�̓K�p
float3 ApplyNoise(float3 color, float2 uv)
{
    float noise = random(uv + time * 0.1) * noiseStrength;
    return color + noise;
}

float4 main(PSInput input) : SV_TARGET
{
    // UV�c�݂�K�p
    float2 distortedUV = ApplyUVGlitch(input.uv);

    // �F������K�p
    float3 color = ApplyChromaticAberration(distortedUV);

    // �����_���u���b�N�m�C�Y�̓K�p
    float blockNoise = ApplyBlockNoise(input.uv);
    color.rgb = lerp(color.rgb, float3(0.5, 0.0, 0.0), blockNoise * glitchIntensity);

    // �ʏ�m�C�Y��K�p
    color = ApplyNoise(color, input.uv);

    return float4(color, 1.0);
}
