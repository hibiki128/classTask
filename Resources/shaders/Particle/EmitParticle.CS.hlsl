#include"Particle.hlsli"
#include"../Random/Random.hlsli"

ConstantBuffer<GPUEmitterData> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

float3 LerpFloat3(float3 a, float3 b, float t)
{
    return a + (b - a) * t;
}

float4 LerpFloat4(float4 a, float4 b, float t)
{
    return a + (b - a) * t;
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint threadIndex = DTid.x;

    // emitされてなければreturn
    if (gEmitter.emit == 0 || threadIndex >= gEmitter.emitCount)
        return;

    RandomGenerator generator;
    generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;

    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

    if (0 <= freeListIndex && freeListIndex < kMaxParticles)
    {
        int particleIndex = gFreeList[freeListIndex];

        // 各種初期化処理
        float3 randomScale = generator.Generate3d();
        gParticles[particleIndex].scale.x = lerp(gEmitter.scaleMin.x, gEmitter.scaleMax.x, randomScale.x);
        gParticles[particleIndex].scale.y = lerp(gEmitter.scaleMin.y, gEmitter.scaleMax.y, randomScale.y);
        gParticles[particleIndex].scale.z = lerp(gEmitter.scaleMin.z, gEmitter.scaleMax.z, randomScale.z);
        gParticles[particleIndex].translate = gEmitter.position;

        if (gEmitter.colorMode == 0)
        {
            gParticles[particleIndex].color.rgb = generator.Generate3d();
            gParticles[particleIndex].color.a = 1.0f;
        }
        else if (gEmitter.colorMode == 1)
        {
            gParticles[particleIndex].color = gEmitter.startColor;
        }
        else
        {
            gParticles[particleIndex].color = gEmitter.startColor;
        }

        float3 randomVel = generator.Generate3d();
        gParticles[particleIndex].velocity.x = lerp(gEmitter.velocityMin.x, gEmitter.velocityMax.x, randomVel.x);
        gParticles[particleIndex].velocity.y = lerp(gEmitter.velocityMin.y, gEmitter.velocityMax.y, randomVel.y);
        gParticles[particleIndex].velocity.z = lerp(gEmitter.velocityMin.z, gEmitter.velocityMax.z, randomVel.z);

        float randomLifeTime = generator.Generate1d();
        gParticles[particleIndex].lifeTime = lerp(gEmitter.lifeTimeMin, gEmitter.lifeTimeMax, randomLifeTime);
        gParticles[particleIndex].currentTime = 0.0f;
    }
    else
    {
        // パーティクルが使えなかった分、リストを戻す
        InterlockedAdd(gFreeListIndex[0], 1);
    }
}
