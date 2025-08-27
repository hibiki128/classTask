#pragma once
#include "type/Matrix4x4.h"
#include "type/Vector3.h"
#include "type/Vector4.h"

/// ===== GPUParticle =====

struct CSParticle {
    Vector3 translate;
    Vector3 scale;
    float lifeTime;
    Vector3 velocity;
    float currentTime;
    Vector4 color;
};

struct PerView {
    Matrix4x4 viewProjection;
    Matrix4x4 billboardMatrix;
};

struct EmitterSphere {
    Vector3 translate;
    float radius;
    uint32_t count;
    float frequency;
    float frequencyTime;
    uint32_t emit;
};

struct PerFrame {
    float time;
    float deltaTime;
};

struct GPUParticleEmitter {
    EmitterSphere *emitter;
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterResource;
    bool isEmit = false;
    int emitterIndex = 0;
};

/// =======================

/// ====== CPUParticle ======

struct ParticleSetting {
    int maxTrailParticles; // 最大軌跡パーティクル数
    float gatherStartRatio = 0.5f;
    float gatherStrength = 2.0f;
    float trailSpawnInterval; // 軌跡パーティクル生成間隔
    float trailLifeScale;     // 軌跡パーティクルの寿命スケール
    float lifeTimeMin;
    float lifeTimeMax;
    float gravity;
    float alphaMin;
    float alphaMax;
    float scaleMin;
    float scaleMax;
    float trailVelocityScale; // 軌跡の速度スケール
    Vector3 translate;
    Vector3 rotation;
    Vector3 scale;
    Vector3 velocityMin;
    Vector3 velocityMax;
    Vector3 particleStartScale;
    Vector3 particleEndScale;
    Vector3 startAcce;
    Vector3 endAcce;
    Vector3 startRote;
    Vector3 endRote;
    Vector3 rotateVelocityMin;
    Vector3 rotateVelocityMax;
    Vector3 allScaleMax;
    Vector3 allScaleMin;
    Vector3 rotateStartMax;
    Vector3 rotateStartMin;
    Vector3 trailScaleMultiplier; // 軌跡パーティクルのサイズ倍率
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 trailColorMultiplier; // 軌跡パーティクルの色倍率
    uint32_t count;
    bool enableTrail;          // 軌跡機能を有効にするか
    bool trailInheritVelocity; // 軌跡が親の速度を継承するか
    bool isRandomColor;
    bool isBillboard = false;
    bool isBillboardX = false;
    bool isBillboardY = false;
    bool isBillboardZ = false;
    bool isRandomRotate = false;
    bool isRotateVelocity = false;
    bool isAcceMultiply = false;
    bool isRandomSize = false;
    bool isRandomAllSize = false;
    bool isSinMove = false;
    bool isFaceDirection = false;
    bool isEndScale = false;
    bool isEmitOnEdge = false;
    bool isGatherMode = false;

    BlendMode blendMode = BlendMode::kAdd;

    ParticleSetting() : enableTrail(false), trailSpawnInterval(0.05f),
                        maxTrailParticles(1), trailLifeScale(0.5f),
                        trailScaleMultiplier({0.8f, 0.8f, 0.8f}),
                        trailColorMultiplier({1.0f, 1.0f, 1.0f, 0.7f}),
                        trailInheritVelocity(true), trailVelocityScale(0.3f) {}
};

struct ParticleStats {
    size_t count = 0;
    size_t instanceCount = 0; // 同じ名前のエミッター数
};

/// =========================