cbuffer cbPerObject
{
    row_major float4x4 WVP;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VOut VShader(float4 position : POSITION, float4 color : COLOR)
{
    VOut output;

    output.position = mul(position, WVP);
    output.color = color;

    return output;
}