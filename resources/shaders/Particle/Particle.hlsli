struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct Particle
{
    float3 translate;
    float3 scale;
    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;
};

struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};

struct PerFrame
{
    float time;
    float deltaTime;
};

static const int kMaxParticles = 100000;

struct GPUEmitterData
{
    float3 position;
    float3 velocityMin;
    float3 velocityMax;
    float3 scaleMin;
    float3 scaleMax;
    float lifeTimeMin;
    float lifeTimeMax;
    int colorMode;
    float4 startColor;
    float4 endColor;
    int emitCount;
    float emitInterval;
    float currentTime;
    int emit;
};