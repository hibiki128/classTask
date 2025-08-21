#pragma once
#include <Primitive/PrimitiveModel.h>
#include <type/Matrix4x4.h>
#include <type/Vector3.h>
#include <type/Vector4.h>

// パーティクルの色設定モード
enum class ColorMode {
    Random = 0,  // ランダム色
    Fixed,       // 固定色
    LifeTimeLerp // 寿命に応じた補間色
};

// パーティクルの色設定モード
enum class ParticleColorMode {
    Random = 0, // ランダム色
    Fixed,      // 固定色
    Lerp        // 寿命に応じた補間色
};

// パーティクルエミッター設定
struct ParticleEmitterSettings {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 velocityMin = {-1.0f, -1.0f, -1.0f};
    Vector3 velocityMax = {1.0f, 1.0f, 1.0f};
    Vector3 scaleMin = {0.1f, 0.1f, 0.1f};
    Vector3 scaleMax = {1.0f, 1.0f, 1.0f};
    float lifeTimeMin = 1.0f;
    float lifeTimeMax = 3.0f;

    // 色設定
    ColorMode colorMode = ColorMode::Random;
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 0.0f};

    // エミッション設定
    uint32_t emitCount = 10;
    float emitInterval = 0.5f;

    bool isActive = true;
    PrimitiveType primitiveType = PrimitiveType::Plane;
};

// パーティクルエミッター設定
struct ParticleCSEmitterSettings {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 velocityMin = {-1.0f, -1.0f, -1.0f};
    Vector3 velocityMax = {1.0f, 1.0f, 1.0f};
    Vector3 scaleMin = {0.1f, 0.1f, 0.1f};
    Vector3 scaleMax = {1.0f, 1.0f, 1.0f};
    float lifeTimeMin = 1.0f;
    float lifeTimeMax = 3.0f;

    // 色設定
    ParticleColorMode colorMode = ParticleColorMode::Lerp;
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 0.0f};

    // エミッション設定
    uint32_t emitCount = 10;
    float emitInterval = 1.0f / 60.0f;

    bool isActive = true;
};

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

struct GPUEmitterData {
    Vector3 position;
    float pad1;
    Vector3 velocityMin;
    float pad2;
    Vector3 velocityMax;
    float pad3;
    Vector3 scaleMin;
    float pad4;
    Vector3 scaleMax;
    float pad5;
    float lifeTimeMin;
    float lifeTimeMax;
    uint32_t colorMode;
    float pad6;
    Vector4 startColor;
    Vector4 endColor;
    uint32_t emitCount;
    float emitInterval;
    float currentTime;
    uint32_t emit;
};

struct PerFrame {
    float time;
    float deltaTime;
};

// パーティクル統計情報
struct ParticleCSStats {
    size_t count = 0;
    size_t instanceCount = 0; // 同じ名前のエミッター数
};

// GPU用パーティクルデータ（描画用）
struct ParticleCSForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};
