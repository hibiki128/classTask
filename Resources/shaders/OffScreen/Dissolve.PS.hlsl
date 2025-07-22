#include"FullScreen.hlsli"

struct DissolveValue
{
    float value;
};

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);
ConstantBuffer<DissolveValue> gDissolveValue : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float mask = gMaskTexture.Sample(gSampler, input.texcoord);
    if(mask < gDissolveValue.value)
    {
        discard;
    };
    
    output.color = gTexture.Sample(gSampler, input.texcoord);
    return output;
}
