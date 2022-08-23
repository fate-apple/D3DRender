#ifndef BRDF_HLSLI
#define BRDF_HLSLI

#include "LightSource.hlsli"
#include "Math.hlsli"

struct SurfaceInfo
{
    // Set from outside.
    float3 P;
    float3 N;
    float3 V;

    float4 albedo;
    float roughness;
    float metallic;
    float3 emission;



    // Inferred from properties above.
    float alphaRoughness;
    float alphaRoughnessSquared;
    float NdotV;
    float3 F0;
    float3 R;

    inline void inferRemainingProperties()
    {
        alphaRoughness = roughness * roughness;
        alphaRoughnessSquared = alphaRoughness * alphaRoughness;
        NdotV = saturate(dot(N, V));
        F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metallic);
        R = reflect(-V, N);
    }
};

// geometry information about light in surface point P ;
struct LightInfo
{
    float3 L;
    float3 H;
    float NdotL;
    float NdotH;
    float LdotH;
    float VdotH;

    float3 radiance;
    float distanceToLight;

    inline void initialize(SurfaceInfo surface, float3 inL, float3 rad)
    {
        L = inL;
        H = normalize(L + surface.V);
        NdotL = saturate(dot(surface.N, L));
        NdotH = saturate(dot(surface.N, H));
        LdotH = saturate(dot(L, H));
        VdotH = saturate(dot(surface.V, H));
        radiance = rad;
    }

    inline void initializeFromPointLight(SurfaceInfo surface, PointLightCB pointLight)
    {
        float3 L = pointLight.position - surface.P;
        distanceToLight = length(L);
        L /= distanceToLight;

        initialize(surface, L, pointLight.radiance * getAttenuation(distanceToLight, pointLight.radius) * LIGHT_RADIANCE_SCALE);
    }
};



// ----------------------------------------
// GEOMETRIC MASKING (Microfacets may shadow each-other).
// ----------------------------------------

static float geometrySmith(float NdotL, float NdotV, float roughness)
{
    float k = (roughness * roughness) * 0.5f;

    float ggx2 = NdotV / (NdotV * (1.f - k) + k);
    float ggx1 = NdotL / (NdotL * (1.f - k) + k);

    return ggx1 * ggx2;
}

static float geometrySmith(SurfaceInfo surface, LightInfo light)
{
    float k = surface.alphaRoughness * 0.5f;

    float ggx2 = surface.NdotV / (surface.NdotV * (1.f - k) + k);
    float ggx1 = light.NdotL / (light.NdotL * (1.f - k) + k);

    return ggx1 * ggx2;
}


// ----------------------------------------
// DISTRIBUTION (Microfacets' orientation based on roughness).
// ----------------------------------------

static float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float d = (NdotH2 * (a2 - 1.f) + 1.f);
    return a2 / max(d * d * M_PI, 0.001f);
}

static float distributionGGX(SurfaceInfo surface, LightInfo light)
{
    float NdotH = light.NdotH;
    float NdotH2 = NdotH * NdotH;
    float a2 = surface.alphaRoughnessSquared;
    float d = (NdotH2 * (a2 - 1.f) + 1.f);
    return a2 / max(d * d * M_PI, 0.001f);
}




static float4 importanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float phi = 2.f * M_PI * Xi.x;
    float cosTheta = sqrt((1.f - Xi.y) / (1.f +(a2 -1.f) * Xi.y));
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    // From spherical coordinates to cartesian coordinates.
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    // From tangent-space vector to world-space sample vector. right-hand. z-up. tbn
    float3 up = abs(N.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    H = tangent * H.x + bitangent * H.y + N * H.z;
    float d = ( a2 - 1) * cosTheta * cosTheta + 1.f;
    float D = a2 / (M_PI * d * d);
    float pdf = D * cosTheta;       //D* NoH, dw
    return float4(normalize(H), pdf); 
}




#endif