#pragma once
#include "Particle/ParticleStruct.h"
#include <Graphics/Srv/SrvManager.h>
#include <Graphics/Texture/TextureManager.h>
#include <Model/ModelStructs.h>
#include <Particle/ParticleCommon.h>
#include <d3d12.h>
#include <utility>
#include <wrl.h>
#include <Camera/ViewProjection/ViewProjection.h>
#include <Model/Model.h>
#include <Primitive/PrimitiveModel.h>

class ParticleCSGroup {
  public:
    ParticleCSGroupData CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath = {});
    ParticleCSGroupData CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, const std::string &texturePath = {});
    void Update(const ViewProjection &vp);
    void DrawImGui();

  private:
    /// ===================================
    /// private methods
    /// ===================================
    void Initialize();
    void InitParticle();
    void UpdateParticleCSDisPatch();
    void CreateOutputParticleResource();
    void CreatePerViewResource();
    void CreateMaterialResource();
    void CreateIndexResource();
    void CreateVertexResource();
    void CreatePerFrameResource();
    void CreateFreeListIndexResource();
    void CreateFreeListResource();

  private:
    /// ===================================
    /// private variaus
    /// ===================================
    Microsoft::WRL::ComPtr<ID3D12Resource> outputParticleResource_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputParticleSrvHandle_;
    uint32_t outputParticleSrvIndex_ = 0;
    uint32_t outputParticleSrvForVSIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    uint32_t *indexData_;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
    PerView *perViewData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
    ParticleMaterial *materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    VertexData *vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_ = nullptr;
    PerFrame *perFrameData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListIndexSrvHandle_;
    uint32_t freeListIndexSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListSrvHandle_;
    uint32_t freeListSrvIndex_ = 0;

    ID3D12GraphicsCommandList *commandList;

    ParticleCommon *particleCommon_;
    DirectXCommon *dxCommon_;
    SrvManager *srvManager_;
    TextureManager *texManager_;
    Model *model_;
    ModelData modelData;
    std::string modelFilePath_;

    PrimitiveType type_ = PrimitiveType::None;
    ParticleCSGroupData particleGroupData_;

    std::string texPath_ = "debug/circle2.png";
};
