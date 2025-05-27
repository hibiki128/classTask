#pragma once
#include "ParticleCommon.h"
#include "Srv/SrvManager.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "ViewProjection/ViewProjection.h"
#include <Matrix4x4.h>
#include <ModelStructs.h>
#include <ParticleGroup.h>
#include <WorldTransform.h>
#include <random>
#include <unordered_map> // 追加

struct ParticleSetting {
    Vector3 translate;
    Vector3 rotation;
    Vector3 scale;
    uint32_t count;
    Vector3 velocityMin;
    Vector3 velocityMax;
    float lifeTimeMin;
    float lifeTimeMax;
    float gravity;
    Vector3 particleStartScale;
    Vector3 particleEndScale;
    Vector3 startAcce;
    Vector3 endAcce;
    Vector3 startRote;
    Vector3 endRote;
    bool isRandomColor;
    float alphaMin;
    float alphaMax;
    Vector3 rotateVelocityMin;
    Vector3 rotateVelocityMax;
    Vector3 allScaleMax;
    Vector3 allScaleMin;
    float scaleMin;
    float scaleMax;
    Vector3 rotateStartMax;
    Vector3 rotateStartMin;
    bool isBillboard = false;
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
    float gatherStartRatio = 0.5f;
    float gatherStrength = 2.0f;
};

class ParticleManager {
  public:
    void Initialize(SrvManager *srvManager);
    void Update(const ViewProjection &viewProjeciton);
    void Draw();
    void AddParticleGroup(ParticleGroup *particleGroup);
    void RemoveParticleGroup(const std::string &name);

    // グループごとのParticleSetting
    void SetParticleSetting(const std::string &groupName, const ParticleSetting &setting);
    ParticleSetting &GetParticleSetting(const std::string &groupName);
    std::vector<std::string> GetParticleGroupsName();

  private:
    ParticleCommon *particleCommon = nullptr;
    SrvManager *srvManager_;
    std::unordered_map<std::string, ParticleGroup *> particleGroups_;
    std::unordered_map<std::string, ParticleSetting> particleSettings_; // ここがポイント
    std::vector<std::string> particleGroupNames_;
    std::random_device seedGenerator;
    std::mt19937 randomEngine;

  public:
    std::list<Particle> Emit();

  private:
    Particle MakeNewParticle(std::mt19937 &randomEngine, const ParticleSetting &setting);
};
