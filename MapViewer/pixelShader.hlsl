struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

static const float3 ambientLight = float3(0.0f, 0.0f, 0.0f);
static const float3 diffuseLight = float3(1.0f, 1.0f, 1.0f);

static const float3 lightDir = normalize(float3(-1.0f, -1.0f, -1.0f));

//vec3 getLightColor(vec3 normal, vec3 worldPos, vec3 shadowPos) {
//    normal = normalize(normal);
//
//    float ndl = dot(normal, -lightDir);
//    vec3 r = normalize(-lightDir - 2.0f * ndl * normal);
//    vec3 v = normalize(worldPos - globalParams.eyePosition.xyz);
//
//    float diffuse = max(ndl * 0.5f + 0.5f, 0);
//    float specular = max(pow(dot(r, v), material.shininess), 0);
//
//    float shadow = sampleShadow(shadowPos, 3);
//    float visibility = shadow * 0.35f + 0.65f;
//
//    vec3 lightColor = material.ambientLight;
//    lightColor.rgb += material.diffuseLight * diffuse;
//    lightColor.rgb += material.specularLight * specular;
//    return lightColor * visibility + material.emissiveLight;
//}

float3 getLightColor(float3 normal, float3 worldPos) {
    float ndl = dot(normalize(normal), -lightDir);
    float diffuse = max(ndl * 0.35f + 0.5f, 0);

    float3 lightColor = ambientLight;
    lightColor.rgb += diffuseLight * diffuse;
    return lightColor;
}

float4 PShader(VSOutput input) : SV_TARGET
{
    float4 color = input.color;
    color.rgb *= getLightColor(input.normal, input.worldPos);
    return color;
}
