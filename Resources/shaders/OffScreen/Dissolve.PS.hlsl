#include"FullScreen.hlsli"

struct Threshold
{
    float dissolveAmount;
};

ConstantBuffer<Threshold> gThreshold : register(b0);
Texture2D gTexture : register(t0);
Texture2D gMaskTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float mask = gMaskTexture.Sample(gSampler, input.texcoord).r;
    if (mask <= gThreshold.dissolveAmount)
    {
        discard;
    }
    output.color = gTexture.Sample(gSampler, input.texcoord);
    return output;
}
