struct EmitterSphere
{
    float3 translate;
    float radius;
    int count;
    float frequency;
    float frequencyTime;
    int emit;
};

ConstantBuffer<EmitterSphere> gEmitter : register(b0);


[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit != 0)
    {
        for (int countIndex = 0; countIndex < gEmitter.count; ++countIndex)
        {
            gParticles[countIndex].scale = float3(0.3f, 0.3f, 0.3f);
            gParticles[countIndex].translate = float3(0.0f, 0.0f, 0.0f);
            gParticles[countIndex].color = float4(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}