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
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : WORLDPOS;
};

PSInput VSTriangle(VSInput input)
{
    PSInput output = (PSInput)0;

    output.position = mul(input.position, world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);

    output.normal = mul(input.normal, ((float3x3) world));

    return output;
}

float4 PSLambert(PSInput input) : SV_TARGET
{
    float4 finalColor = 0;

    for (int i = 0; i < 2; i++)
    {
        finalColor += saturate(dot((float3) lightDir[i], input.normal) * lightColor[i]);
    }
    finalColor.a = 1;

    return finalColor;
}

float4 PSSolid(PSInput input) : SV_TARGET
{
    return outputColor;
}