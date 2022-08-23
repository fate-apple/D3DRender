#include "cs.hlsli"
#include "PostProcessing_rs.hlsli"
#include "ShadowBlurCommon.hlsli"

[RootSignature(SHADOW_BLUR_X_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CsInput In)
{
    float value = blur(float2(1.f, 0.f), In.dispatchThreadID.xy);
    output[In.dispatchThreadID.xy] = value;
}
