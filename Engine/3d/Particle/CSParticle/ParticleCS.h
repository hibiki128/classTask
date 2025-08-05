#pragma once
#include "Model/ModelStructs.h"
#include "Particle/ParticleCommon.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include "wrl.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <Graphics/Srv/SrvManager.h>
#include <Graphics/Texture/TextureManager.h>
#include <d3d12.h>
#include <type/Matrix4x4.h>
#include <utility>

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

class DirectXCommon;
class ParticleCS {

  public:
    void Initialize();
    void Draw(const ViewProjection &vp);

  private:
    void EmitterUpdate();
    void Update();
    void CreateOutputParticleResource();
    void CreatePerViewResource();
    void CreateMaterialResource();
    void CreateIndexResource();
    void CreateEmitterSphereResource();
    void CreateVertexResource();

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> outputParticleResource;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputParticleSrvHandle;
    uint32_t outputParticleSrvIndex_ = 0;
    uint32_t outputParticleSrvForVSIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
    uint32_t *indexData;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource;
    PerView *perViewData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    ParticleMaterial *materialData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    VertexData *vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> emitterSphereResource_ = nullptr;
    EmitterSphere *emitterSphereData_ = nullptr;

    ParticleCommon *particleCommon_;
    DirectXCommon *dxCommon_;
    SrvManager *srvManager_;
    TextureManager *texManager_;

    std::string texPath_ = "debug/circle2.png";
};
