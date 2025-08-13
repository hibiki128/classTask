#include"Particle.hlsli"
#include"../Random/Random.hlsli"

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeCounter : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    RandomGenerator generator;
    generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;
    if (gEmitter.emit != 0)
    {
        for (int countIndex = 0; countIndex < gEmitter.count; ++countIndex)
        {
            int particleIndex;
            InterlockedAdd(gFreeCounter[0], 1, particleIndex);
            if (particleIndex < kMaxParticles)
            {
                gParticles[particleIndex].scale = generator.Generate3d();
                gParticles[particleIndex].translate = generator.Generate3d();
                gParticles[particleIndex].color.rgb = generator.Generate3d();
                gParticles[particleIndex].color.a = 1.0f;
                float3 vel = (generator.Generate3d() * 2.0f - 1.0f) * 0.25f;
                gParticles[particleIndex].velocity = vel;
                gParticles[particleIndex].lifeTime = 3.0f;
                gParticles[particleIndex].currentTime = 0.0f;
            }
        }
    }
}