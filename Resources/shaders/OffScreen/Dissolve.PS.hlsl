#include"FullScreen.hlsli"

struct Threshold
{
    float dissolveAmount;
};

ConstantBuffer<Threshold> gThreshold : register(b0);
Texture2D<float4> gTexture : register(t0);
Texture2D<float> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float mask = gMaskTexture.Sample(gSampler, input.texcoord);
    if (mask <= gThreshold.dissolveAmount)
    {
        discard;
    }
    //float edge = 1.0f - smoothstep(0.5f, 0.53f, mask);
    output.color = gTexture.Sample(gSampler, input.texcoord);
   // output.color.rgb += edge * float3(1.0f, 0.4f, 0.4f);
    return output;
}
