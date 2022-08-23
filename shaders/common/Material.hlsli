#ifndef MATERIAL_H
#define MATERIAL_H


static uint32 packColor(uint32 r, uint32 g, uint32 b, uint32 a)
{
    return ((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);
}

static uint32 packColor(float r, float g, float b, float a)
{
    return packColor(
        (uint32)clamp(r * 255.f, 0.f, 255.f),
        (uint32)clamp(g * 255.f, 0.f, 255.f),
        (uint32)clamp(b * 255.f, 0.f, 255.f),
        (uint32)clamp(a * 255.f, 0.f, 255.f));
}

static uint32 packColor(vec4 rgba)
{
    return packColor(rgba.r, rgba.g, rgba.b, rgba.a);
}

static vec4 unpackColor(uint32 c)
{
    const float mul = 1.f / 255.f;
    float r = (c & 0xFF) * mul;
    float g = ((c >> 8) & 0xFF) * mul;
    float b = ((c >> 16) & 0xFF) * mul;
    float a = ((c >> 24) & 0xFF) * mul;
    return vec4(r, g, b, a);
}

struct PbrDecalCB
{
    vec3 position;
    uint32 albedoTint; // RGBA packed into one uint32.
    vec3 right; // Scaled by half dimension.
    uint32 roughnessOverride_metallicOverride;
    vec3 up; // Scaled by half dimension.
    uint32 viewportXY; // Top left corner packed into 16 bits each.
    vec3 forward; // Scaled by half dimension.
    uint32 viewportScale; // Width and height packed into 16 bits each.


    void initialize(vec3 position_, vec3 right_, vec3 up_, vec3 forward_, vec4 albedo_, float roughness_, float metallic_,
                    vec4 viewport_)
    {
        position = position_;
        right = right_;
        up = up_;
        forward = forward_;

        albedoTint = packColor(albedo_);

        uint32 r = (uint32)(roughness_ * 0xFFFF);
        uint32 m = (uint32)(metallic_ * 0xFFFF);
        roughnessOverride_metallicOverride = (r << 16) | m;

        uint32 x = (uint32)(viewport_.x * 0xFFFF);
        uint32 y = (uint32)(viewport_.y * 0xFFFF);
        uint32 w = (uint32)(viewport_.z * 0xFFFF);
        uint32 h = (uint32)(viewport_.w * 0xFFFF);
        viewportXY = (x << 16) | y;
        viewportScale = (w << 16) | h;
    }

#ifndef HLSL
    pbr_decal_cb() {}

    pbr_decal_cb(vec3 position_, vec3 right_, vec3 up_, vec3 forward_, vec4 albedo_, float roughness_, float metallic_, vec4 viewport_)
    {
        initialize(position_, right_, up_, forward_, albedo_, roughness_, metallic_, viewport_);
    }
#endif

    vec4 getAlbedo()
    {
        return unpackColor(albedoTint);
    }

    float getRoughnessOverride()
    {
        return (roughnessOverride_metallicOverride >> 16) / (float)0xFFFF;
    }

    float getMetallicOverride()
    {
        return (roughnessOverride_metallicOverride & 0xFFFF) / (float)0xFFFF;
    }

    vec4 getViewport()
    {
        float x = (viewportXY >> 16) / (float)0xFFFF;
        float y = (viewportXY & 0xFFFF) / (float)0xFFFF;
        float w = (viewportScale >> 16) / (float)0xFFFF;
        float h = (viewportScale & 0xFFFF) / (float)0xFFFF;
        return vec4(x, y, w, h);
    }
};

struct PbrMaterialCB
{
    vec3 emission;
    uint32 albedoTint;
    uint32 roughness_metallic_flags_refraction;
    uint32 normalMapStrength_doubleSided_uvScale;

    void Initialize(vec4 inAlbedo, vec3 inEmission, float inRoughness, float inMetallic, uint32 flags,
                    float normalMapStrength = 1.f,
                    float refractionStrength = 1.f, bool doubleSide = false, float uvScale = 1.f)
    {
        emission = inEmission;
        albedoTint = packColor(inAlbedo);
        inRoughness = clamp(inRoughness, 0.f, 1.f);
        inMetallic = clamp(inMetallic, 0.f, 1.f);
        normalMapStrength = clamp(normalMapStrength, 0.f, 1.f);
        refractionStrength = clamp(refractionStrength, 0.f, 1.f);
        roughness_metallic_flags_refraction =   (uint32)(inRoughness * 0xff) << 24 |
                                                (uint32)(inMetallic * 0xff) << 16 |
                                                (uint32)(flags << 8) |
                                                (uint32)(refractionStrength * 0xff);
#ifndef HLSL
        uint32 uvScaleHalf = half(uvScale).h;       //custom operator float()
#else
        uint32 uvScaleHalf = f32tof16(uvScale);
#endif
        normalMapStrength_doubleSided_uvScale =
            ((uint32)(normalMapStrength * 0xFF) << 24) |
            ((uint32)(doubleSide) << 16) |
            uvScaleHalf;
    }

#ifndef HLSL
    PbrMaterialCB() {}

    PbrMaterialCB(vec4 albedo_, vec3 emission_, float roughness_, float metallic_, uint32 flags_, float normalMapStrength_ = 1.f, float refractionStrength_ = 0.f, bool doubleSided_ = false, float uvScale_ = 1.f)
    {
        Initialize(albedo_, emission_, roughness_, metallic_, flags_, normalMapStrength_, refractionStrength_, doubleSided_, uvScale_);
    }
#endif

    vec4 getAlbedo()
    {
        return unpackColor(albedoTint);
    }

    float getRoughnessOverride()
    {
        return ((roughness_metallic_flags_refraction >> 24) & 0xFF) / (float)0xFF;
    }

    float getMetallicOverride()
    {
        return ((roughness_metallic_flags_refraction >> 16) & 0xFF) / (float)0xFF;
    }

    float getNormalMapStrength()
    {
        return ((normalMapStrength_doubleSided_uvScale >> 24) & 0xFF) / (float)0xFF;
    }

    bool doubleSided()
    {
        return ((normalMapStrength_doubleSided_uvScale >> 16) & 0xFF) != 0;
    }

    float uvScale()
    {
        uint32 uvScaleHalf = normalMapStrength_doubleSided_uvScale & 0xFFFF;
#ifndef HLSL
        return half((uint16)uvScaleHalf);
#else
        return f16tof32(uvScaleHalf);
#endif
    }

    float getRefractionStrength()
    {
        return ((roughness_metallic_flags_refraction >> 0) & 0xFF) / (float)0xFF;
    }

    uint32 getFlags()
    {
        return (roughness_metallic_flags_refraction >> 8) & 0xFF;
    }
};


struct LightContribution
{
    float3 diffuse;
    float3 specular;

    void add(LightContribution other, float visibility)
    {
        diffuse += other.diffuse * visibility;
        specular += other.specular * visibility;
    }

    float4 evaluate(float4 albedo)
    {
        float3 c = albedo.rgb * diffuse + specular;
        return float4(c, albedo.a);
    }
};


#define USE_ALBEDO_TEXTURE			(1 << 0)
#define USE_NORMAL_TEXTURE			(1 << 1)
#define USE_ROUGHNESS_TEXTURE		(1 << 2)
#define USE_METALLIC_TEXTURE		(1 << 3)
#define USE_AO_TEXTURE				(1 << 4)
#define USE_32_BIT_INDICES			(1 << 5)    // Ugly to put this in the material. Only used in the ray tracing shaders.

#endif