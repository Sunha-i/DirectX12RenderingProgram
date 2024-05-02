cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    float4 lightDir[2];
    float4 lightColor[2];
    float4 outputColor;
}

struct VSInput
{
    float4 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : WORLDPOS;
};

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

PSInput VSTriangle(VSInput input)
{
    PSInput output = (PSInput)0;

    output.position = mul(input.position, world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);

    output.normal = mul(input.normal, ((float3x3) world));
    output.texCoord = input.texCoord;

    return output;
}

float4 PSLambert(PSInput input) : SV_TARGET
{
    return txDiffuse.Sample(samLinear, input.texCoord);
}

float4 PSSolid(PSInput input) : SV_TARGET
{
    return outputColor;
}