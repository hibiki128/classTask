#pragma once
#include "Particle/ParticleStruct.h"
#include "ParticleCSGroup.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <DirectXCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <Particle/ParticleCommon.h>
#include <vector>

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
    void DrawEmitter();

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
    bool isVisible_ = true;
};
