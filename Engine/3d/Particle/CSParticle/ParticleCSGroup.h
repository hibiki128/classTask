#pragma once

#include "Model/Model.h"
#include "Model/ModelStructs.h"
#include "Particle/ParticleCommon.h"
#include "Primitive/PrimitiveModel.h"
#include "Transform/WorldTransform.h"
#include "type/Matrix4x4.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include "wrl.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <Graphics/Srv/SrvManager.h>
#include <Graphics/Texture/TextureManager.h>
#include <d3d12.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include"Particle/ParticleStruct.h"

class DirectXCommon;

class ParticleCSGroup {
  public:
    ParticleCSGroup();
    ~ParticleCSGroup();

    // 初期化（モデル使用）
    void InitializeWithModel(const std::string &groupName, const std::string &modelPath, const std::string &texturePath = "");

    // 初期化（プリミティブ使用）
    void InitializeWithPrimitive(const std::string &groupName, PrimitiveType primitiveType, const std::string &texturePath = "");

    // 更新
    void Update();

    // 描画
    void Draw(const ViewProjection &vp);

    // エミッター管理
    uint32_t AddEmitter(const ParticleCSEmitterSettings &settings);
    void RemoveEmitter(uint32_t emitterId);
    void UpdateEmitterSettings(uint32_t emitterId, const ParticleCSEmitterSettings &settings);

    // パーティクル制御
    void EmitParticles();
    void ResetParticles();

    // ゲッター
    size_t GetActiveParticleCount() const;
    std::string GetGroupName() const;
    std::string GetModelPath() const;
    std::string GetTexturePath() const;
    PrimitiveType GetPrimitiveType() const;

  private:
    // 共通初期化
    void InitializeCommon(const std::string &groupName, const std::string &texturePath);

    // リソース作成
    void CreateParticleResources();
    void CreateVertexAndIndexResources();
    void CreateDefaultQuad();
    void CreateMaterialResource();

    // エミッター更新
    void UpdateEmitters();
    void UpdateParticles();
    void UpdateGPUEmitterData(uint32_t emitterId);

    // パーティクル初期化
    void InitializeParticles();

  private:
    // 基本オブジェクト
    DirectXCommon *dxCommon_;
    ParticleCommon *particleCommon_;
    SrvManager *srvManager_;
    ID3D12GraphicsCommandList *commandList_;

    // グループ情報
    std::string groupName_;
    std::string modelPath_;
    std::string texturePath_;
    PrimitiveType primitiveType_;

    // モデルデータ
    Model *model_ = nullptr;
    ModelData modelData_;

    // パーティクル用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> outputParticleResource_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputParticleSrvHandle_;
    uint32_t outputParticleSrvIndex_ = 0;
    uint32_t outputParticleSrvForVSIndex_ = 0;

    // フリーリストリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListSrvHandle_;
    uint32_t freeListSrvIndex_ = 0;

    // フリーリストインデックスリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListIndexSrvHandle_;
    uint32_t freeListIndexSrvIndex_ = 0;

    // 頂点・インデックスリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    VertexData *vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;
    uint32_t *indexData_ = nullptr;

    // ビュー・マテリアルリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
    PerView *perViewData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    ParticleMaterial *materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
    PerFrame *perFrameData_ = nullptr;

    // エミッター管理
    std::vector<ParticleCSEmitterSettings> emitterSettings_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> emitterResources_;
    std::vector<GPUEmitterData *> emitterDataPtrs_;
    std::vector<float> emitterTimers_;

    // パーティクル情報
    static const uint32_t kMaxParticleCount = 100000;
    uint32_t activeParticleCount_;
    uint32_t maxParticleCount_;
};