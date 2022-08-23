#ifndef NORMAL_H
#define NORMAL_H


// https://aras-p.info/texts/CompactNormalStorage.html#method07stereo
// Stereographic Transform at point(0, 0, -1)
static float2 packNormal(float3 n)
{
    float2 enc = n.xy / (n.z + 1.f);
    const float scale = 1.f / 1.7777f;
    enc *= scale;
    enc = enc * 0.5f + 0.5f;

    return enc;
}

static float3 unpackNormal(float2 enc)
{
    enc = enc * 2 - vec2(1, 1);
    const float scale = 1.7777f;
    enc = enc * scale;
    float3 nn = float3(enc, 1.f);
    float g = 2.f / dot(nn, nn);

    return (g != g) ? float3(0.f, 0.f, -1.f) : float3(g * nn.xy, g - 1.f);
}

static float3 SampleNormalMap(Texture2D<float3> normalMap, SamplerState sampler, float2 uv)
{
    float3 N = normalMap.Sample(sampler, uv).xyz;
    bool reconstructZ = (N.z == 0.f);
    N = N * 2.f - 1.f;
    if(reconstructZ) {
        N.z = sqrt(1.f - dot(N.xy, N.xy));
    }
    return N;
}

#endif