#include "Particle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        if (gParticles[particleIndex].color.a > 0)
        {
            // パーティクル更新
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
            
            // アルファ値を寿命に基づいて計算
            float normalizedTime = gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime;
            float alpha = 1.0f - normalizedTime;
            gParticles[particleIndex].color.a = saturate(alpha);
            
            // 重力の適用（オプション）
            gParticles[particleIndex].velocity.y -= 9.8f * gPerFrame.deltaTime * 0.1f; // 軽い重力
        }
        
        // パーティクルが死んだ場合の処理
        if (gParticles[particleIndex].color.a <= 0)
        {
            gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
            gParticles[particleIndex].color.a = 0;
            
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
            if ((freeListIndex + 1) < kMaxParticles)
            {
                gFreeList[freeListIndex + 1] = particleIndex;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            }
        }
    }
}