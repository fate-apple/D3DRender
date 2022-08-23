#pragma once
#include "core/reflect.h"


struct Bloom_Settings
{
    float threshold = 100.f;
    float strength = 0.1f;
};
REFLECT_STRUCT(Bloom_Settings,
    (threshold, "Threshold"),
    (strength, "Strength")
);

struct Sharpen_Settings
{
    float strength = 0.5f;
};
REFLECT_STRUCT(Sharpen_Settings,
    (strength, "Strength")
);

//Filmic tone mapping.
//TonemapCommon.ush in unreal is too complex to me
struct ToneMapSettings
{
    float ss = 0.22f; //shoulder strength
    float ls = 0.3f; //Linear strength
    float la = 0.1f; //Linear angle
    float ts = 0.2f; //Toe strength
    float tn = 0.01f; //Toe numerator
    float td = 0.3f; //Toe denominator
    float linearWhite = 11.2f;
    float exposure = 0.2f;
    float ToneMap(float color) const
    {
        color *= exp2(exposure);
        return Evaluate(color) / Evaluate(linearWhite);
    }
    float Evaluate(float x) const
    {
        return (x * (ss * x + ls * la) + ts * tn) / (x * (ss * x + ls * 1) + ts * td) - (tn / td);
    }
};
REFLECT_STRUCT(ToneMapSettings, (ss), (ls), (la), (ts), (tn), (td), (linearWhite), (exposure));  