#pragma once
#include "Particle/ParticleCommon.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include "wrl.h"
#include <DirectXCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <d3d12.h>
#include <type/Matrix4x4.h>
#include <utility>
#include <Camera/ViewProjection/ViewProjection.h>
#include <Graphics/Texture/TextureManager.h>
#include"Model/ModelStructs.h"

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

class ParticleCS {

  public:
    void Initialize();
    void Draw(const ViewProjection& vp);
  private:
    void Update();
    void CreateOutputParticleResource();
    void CreatePerViewResource();
    void CreateMaterialResource();

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> outputParticleResource;
    D3D12_VERTEX_BUFFER_VIEW outputParticleBufferView;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputParticleSrvHandle;
    uint32_t outputParticleSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource;
    PerView *perViewData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    ParticleMaterial *materialData = nullptr;

    ParticleCommon *particleCommon_;
    DirectXCommon *dxCommon_;
    SrvManager *srvManager_;
    TextureManager *texManager_;

    std::string texPath_ = "debug/white1x1.png";
};
