#pragma once
#include "Particle/ParticleStruct.h"
#include "ParticleCSGroup.h"
#include <DirectXCommon.h>
#include <Particle/ParticleCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <vector>
#include <Camera/ViewProjection/ViewProjection.h>

class ParticleCSEmitter {

  public:
    /// ==============================================
    /// public methods
    /// ==============================================
    void Initialize();
    void Update();
    void Draw(const ViewProjection &vp);
    void DrawImGui();
    void AddParticleGroup(ParticleCSGroup *particleGroup);

  private:
    /// ==============================================
    /// private methods
    /// ==============================================

    void CreateEmitterSphereResource();
    void EmitterUpdate();
    void EmitterDisPatch();

  private:
    /// ==============================================
    /// private variaus
    /// ==============================================

    Microsoft::WRL::ComPtr<ID3D12Resource> emitterSphereResource_ = nullptr;
    EmitterSphere *emitterSphereData_ = nullptr;

    DirectXCommon *dxCommon_ = nullptr;
    ID3D12GraphicsCommandList *commandList = nullptr;
    ParticleCommon *particleCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;

    std::vector<ParticleCSGroup *> particleGroups_;

    bool isAuto_ = false;
};
