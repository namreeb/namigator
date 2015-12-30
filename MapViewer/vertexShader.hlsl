cbuffer cbPerObject {
    // Why is this all in one matrix? Should have model, view and projection seperate
    row_major float4x4 WVP;
};

struct VSInput {
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

VSOutput VShader(VSInput input)
{
    VSOutput output = (VSOutput)0;
    output.position = mul(float4(input.position, 1.0f), WVP);
    output.worldPos = input.position;
    output.normal = input.normal;
    output.color = input.color;

    return output;
}