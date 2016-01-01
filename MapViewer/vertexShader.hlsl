cbuffer cbPerObject {
    row_major float4x4 viewMatrix;
    row_major float4x4 projMatrix;
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
    float4 position = float4(input.position, 1.0f);
    position = mul(position, viewMatrix);
    position = mul(projMatrix, position);

    VSOutput output = (VSOutput)0;
    output.position = position;
    output.worldPos = input.position;
    output.normal = input.normal;
    output.color = input.color;

    return output;
}