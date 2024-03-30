cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput output = (PSInput)0;

    output.position = mul(position, world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);
    output.color = color;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}