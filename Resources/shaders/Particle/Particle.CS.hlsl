#include"Particle.hlsli"

static const int kMaxParticles = 1024;
RWStructuredBuffer<Particle> gParticles : register(u0);
[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        // Particle構造体の全要素を0で埋めるという書き方
        gParticles[particleIndex] = (Particle) 0;
        gParticles[particleIndex].scale = float3(0.5f, 0.5f, 0.5f);
        gParticles[particleIndex].color = float4(1.0f, 1.0f, 1.0f, 1.0f);

    }

}