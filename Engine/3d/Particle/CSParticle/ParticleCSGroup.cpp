#include "ParticleCSGroup.h"
#include "Frame.h"
#include "Graphics/Model/ModelManager.h"
#include "Graphics/Srv/SrvManager.h"
#include "Graphics/Texture/TextureManager.h"

ParticleCSGroup::ParticleCSGroup() {
    dxCommon_ = nullptr;
    particleCommon_ = nullptr;
    srvManager_ = nullptr;
    commandList_ = nullptr;

    groupName_ = "";
    modelPath_ = "";
    texturePath_ = "";
    primitiveType_ = PrimitiveType::None;

    activeParticleCount_ = 0;
    maxParticleCount_ = kMaxParticleCount;
}

ParticleCSGroup::~ParticleCSGroup() {}

void ParticleCSGroup::InitializeWithModel(const std::string &groupName, const std::string &modelPath, const std::string &texturePath) {
    InitializeCommon(groupName, texturePath);

    modelPath_ = modelPath;
    primitiveType_ = PrimitiveType::None;

    // モデルの読み込み
    ModelManager::GetInstance()->LoadModel(modelPath);
    model_ = ModelManager::GetInstance()->FindModel(modelPath);

    if (model_) {
        modelData_ = model_->GetModelData();
        CreateVertexAndIndexResources();
    }

    CreateMaterialResource();
}

void ParticleCSGroup::InitializeWithPrimitive(const std::string &groupName, PrimitiveType primitiveType, const std::string &texturePath) {
    InitializeCommon(groupName, texturePath);

    modelPath_ = "";
    primitiveType_ = primitiveType;

    // プリミティブモデルの作成
    std::string primitiveModelPath = ModelManager::GetInstance()->CreatePrimitiveModel(primitiveType, texturePath);
    model_ = ModelManager::GetInstance()->FindModel(primitiveModelPath);

    if (model_) {
        modelData_ = model_->GetModelData();
        CreateVertexAndIndexResources();
    }

    CreateMaterialResource();
}

void ParticleCSGroup::InitializeCommon(const std::string &groupName, const std::string &texturePath) {
    groupName_ = groupName;
    texturePath_ = texturePath;

    // 基本的な初期化
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    particleCommon_ = ParticleCommon::GetInstance();
    srvManager_ = SrvManager::GetInstance();
    commandList_ = dxCommon_->GetCommandList().Get();

    // テクスチャの読み込み
    if (!texturePath_.empty()) {
        TextureManager::GetInstance()->LoadTexture(texturePath_);
    }

    // パーティクル用リソースの作成
    CreateParticleResources();

    // デフォルトエミッター設定の追加
    ParticleCSEmitterSettings defaultSettings;
    defaultSettings.position = {0.0f, 0.0f, 0.0f};
    defaultSettings.velocityMin = {-1.0f, -1.0f, -1.0f};
    defaultSettings.velocityMax = {1.0f, 1.0f, 1.0f};
    defaultSettings.scaleMin = {0.5f, 0.5f, 0.5f};
    defaultSettings.scaleMax = {1.0f, 1.0f, 1.0f};
    defaultSettings.lifeTimeMin = 1.0f;
    defaultSettings.lifeTimeMax = 3.0f;
    defaultSettings.colorMode = ParticleColorMode::Lerp;
    defaultSettings.startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    defaultSettings.endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    defaultSettings.emitCount = 10;
    defaultSettings.emitInterval = 1.0f / 60.0f;
    defaultSettings.isActive = true;

    AddEmitter(defaultSettings);

    // パーティクルの初期化処理
    InitializeParticles();
}

void ParticleCSGroup::CreateParticleResources() {
    // 出力パーティクルリソースの作成
    outputParticleResource_ = dxCommon_->CreateBufferResource(sizeof(CSParticle) * kMaxParticleCount, true);

    // UAV用のインデックス
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource_.Get(), kMaxParticleCount, sizeof(CSParticle));

    // SRV用のインデックス（頂点シェーダー用）
    outputParticleSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(outputParticleSrvForVSIndex_, outputParticleResource_.Get(), kMaxParticleCount, sizeof(CSParticle));

    // フリーリストリソース
    freeListResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * kMaxParticleCount, true);
    freeListSrvIndex_ = srvManager_->Allocate() + 1;
    freeListSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListSrvIndex_);
    freeListSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListSrvIndex_, freeListResource_.Get(), kMaxParticleCount, sizeof(uint32_t));

    freeListIndexResource_ = dxCommon_->CreateBufferResource(sizeof(int) * kMaxParticleCount, true);
    freeListIndexSrvIndex_ = srvManager_->Allocate() + 1;
    freeListIndexSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListIndexSrvIndex_);
    freeListIndexSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListIndexSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListIndexSrvIndex_, freeListIndexResource_.Get(), kMaxParticleCount, sizeof(int));

    // PerViewリソース
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerView));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void **>(&perViewData_));
    perViewData_->viewProjection = MakeIdentity4x4();
    perViewData_->billboardMatrix = MakeIdentity4x4();

    // PerFrameリソース
    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void **>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = 0.0f;
}

void ParticleCSGroup::CreateVertexAndIndexResources() {
    if (!model_ || modelData_.meshes.empty()) {
        // デフォルトクアッドの作成
        CreateDefaultQuad();
        return;
    }

    // 複数メッシュ対応: 全メッシュの頂点を連結
    std::vector<VertexData> allVertices;
    for (const auto &mesh : modelData_.meshes) {
        allVertices.insert(allVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    }

    vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * allVertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * allVertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
    std::memcpy(vertexData_, allVertices.data(), sizeof(VertexData) * allVertices.size());

    // インデックスリソースの作成
    std::vector<uint32_t> allIndices;
    uint32_t vertexOffset = 0;
    for (const auto &mesh : modelData_.meshes) {
        for (auto idx : mesh.indices) {
            allIndices.push_back(idx + vertexOffset);
        }
        vertexOffset += static_cast<uint32_t>(mesh.vertices.size());
    }

    indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * allIndices.size());
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * allIndices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
    std::memcpy(indexData_, allIndices.data(), sizeof(uint32_t) * allIndices.size());
}

void ParticleCSGroup::CreateDefaultQuad() {
    // デフォルトクアッド用の頂点データ
    std::vector<VertexData> vertices = {
        {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
        {{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 左上
        {{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},  // 右下
        {{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}    // 右上
    };

    vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * vertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
    std::memcpy(vertexData_, vertices.data(), sizeof(VertexData) * vertices.size());

    // インデックスデータ
    std::vector<uint32_t> indices = {
        0, 1, 2, // 最初の三角形
        2, 1, 3  // 二番目の三角形
    };

    indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * indices.size());
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * indices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
    std::memcpy(indexData_, indices.data(), sizeof(uint32_t) * indices.size());
}

void ParticleCSGroup::CreateMaterialResource() {
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleMaterial));
    materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->uvTransform = MakeIdentity4x4();
}

void ParticleCSGroup::InitializeParticles() {
    srvManager_->SetDescriptorHeap();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    // InitParticle.CSの処理
    particleCommon_->ComputeInitDrawCommonSetting();
    commandList_->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList_->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList_->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);

    const uint32_t threadsPerGroup = 1024;
    uint32_t dispatchCount = (kMaxParticleCount + threadsPerGroup - 1) / threadsPerGroup;
    commandList_->Dispatch(dispatchCount, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCSGroup::Update() {
    perFrameData_->time += Frame::DeltaTime();
    perFrameData_->deltaTime = Frame::DeltaTime();

    // エミッター更新
    UpdateEmitters();

    // パーティクル更新処理
    UpdateParticles();
}

void ParticleCSGroup::UpdateEmitters() {
    float deltaTime = Frame::DeltaTime();

    for (size_t i = 0; i < emitterSettings_.size(); ++i) {
        if (!emitterSettings_[i].isActive)
            continue;

        emitterTimers_[i] += deltaTime;

        GPUEmitterData *data = emitterDataPtrs_[i];
        data->currentTime = emitterTimers_[i];

        if (emitterTimers_[i] >= emitterSettings_[i].emitInterval) {
            emitterTimers_[i] -= emitterSettings_[i].emitInterval;
            data->emit = 1;
        } else {
            data->emit = 0;
        }
    }
}

void ParticleCSGroup::UpdateParticles() {
    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    // 各エミッターに対してEmitParticle実行
    particleCommon_->ComputeEmitterDrawCommonSetting();

    for (size_t i = 0; i < emitterResources_.size(); ++i) {
        if (!emitterSettings_[i].isActive)
            continue;

        commandList_->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
        commandList_->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
        commandList_->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
        commandList_->SetComputeRootConstantBufferView(3, emitterResources_[i]->GetGPUVirtualAddress());
        commandList_->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());

        uint32_t emitCount = emitterSettings_[i].emitCount;
        uint32_t threadGroupSize = 64; // numthreadsのxの数
        uint32_t dispatchCount = (emitCount + threadGroupSize - 1) / threadGroupSize;

        commandList_->Dispatch(dispatchCount, 1, 1);
    }

    // UpdateParticle実行
    particleCommon_->ComputeUpdateEmitterDrawCommonSetting();
    commandList_->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList_->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList_->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    commandList_->SetComputeRootConstantBufferView(3, perFrameResource_->GetGPUVirtualAddress());

    const uint32_t threadsPerGroup = 1024;
    uint32_t dispatchCount = (kMaxParticleCount + threadsPerGroup - 1) / threadsPerGroup;
    commandList_->Dispatch(dispatchCount, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCSGroup::Draw(const ViewProjection &vp) {
    // ビュープロジェクション行列の更新
    perViewData_->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData_->billboardMatrix = vp.matView_;
    perViewData_->billboardMatrix.m[3][0] = 0.0f;
    perViewData_->billboardMatrix.m[3][1] = 0.0f;
    perViewData_->billboardMatrix.m[3][2] = 0.0f;
    perViewData_->billboardMatrix.m[3][3] = 1.0f;
    perViewData_->billboardMatrix = Inverse(perViewData_->billboardMatrix);

    // 描画設定
    particleCommon_->DrawCommonSetting(BlendMode::kAdd);

    commandList_->IASetIndexBuffer(&indexBufferView_);
    commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList_->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
    srvManager_->SetGraphicsRootDescriptorTable(1, outputParticleSrvForVSIndex_);

    // テクスチャの設定
    uint32_t textureIndex = 0;
    if (!texturePath_.empty()) {
        textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(texturePath_);
    }
    srvManager_->SetGraphicsRootDescriptorTable(2, textureIndex);

    commandList_->SetGraphicsRootConstantBufferView(3, materialResource_->GetGPUVirtualAddress());

    // インデックス数の計算
    uint32_t indexCount = 6; // デフォルトクアッド
    if (!modelData_.meshes.empty()) {
        indexCount = 0;
        for (const auto &mesh : modelData_.meshes) {
            indexCount += static_cast<uint32_t>(mesh.indices.size());
        }
    }

    commandList_->DrawIndexedInstanced(indexCount, kMaxParticleCount, 0, 0, 0);
}

void ParticleCSGroup::EmitParticles() {
    // エミッターが自動的にエミットするかの設定
    for (size_t i = 0; i < emitterSettings_.size(); ++i) {
        if (emitterSettings_[i].isActive) {
            GPUEmitterData *data = emitterDataPtrs_[i];
            data->emit = 1; // 強制エミット
        }
    }
}

uint32_t ParticleCSGroup::AddEmitter(const ParticleCSEmitterSettings &settings) {
    uint32_t emitterId = static_cast<uint32_t>(emitterSettings_.size());

    emitterSettings_.push_back(settings);
    emitterTimers_.push_back(0.0f);

    // GPU用リソース作成
    auto resource = dxCommon_->CreateBufferResource(sizeof(GPUEmitterData));
    emitterResources_.push_back(resource);

    GPUEmitterData *dataPtr;
    resource->Map(0, nullptr, reinterpret_cast<void **>(&dataPtr));
    emitterDataPtrs_.push_back(dataPtr);

    UpdateEmitterSettings(emitterId, settings);

    return emitterId;
}

void ParticleCSGroup::RemoveEmitter(uint32_t emitterId) {
    if (emitterId >= emitterSettings_.size())
        return;

    emitterSettings_.erase(emitterSettings_.begin() + emitterId);
    emitterTimers_.erase(emitterTimers_.begin() + emitterId);
    emitterResources_.erase(emitterResources_.begin() + emitterId);
    emitterDataPtrs_.erase(emitterDataPtrs_.begin() + emitterId);
}

void ParticleCSGroup::UpdateEmitterSettings(uint32_t emitterId, const ParticleCSEmitterSettings &settings) {
    if (emitterId >= emitterSettings_.size())
        return;

    emitterSettings_[emitterId] = settings;
    UpdateGPUEmitterData(emitterId);
}

void ParticleCSGroup::UpdateGPUEmitterData(uint32_t emitterId) {
    if (emitterId >= emitterSettings_.size())
        return;

    const auto &settings = emitterSettings_[emitterId];
    GPUEmitterData *data = emitterDataPtrs_[emitterId];

    data->position = settings.position;
    data->velocityMin = settings.velocityMin;
    data->velocityMax = settings.velocityMax;
    data->scaleMin = settings.scaleMin;
    data->scaleMax = settings.scaleMax;
    data->lifeTimeMin = settings.lifeTimeMin;
    data->lifeTimeMax = settings.lifeTimeMax;
    data->colorMode = static_cast<uint32_t>(settings.colorMode);
    data->startColor = settings.startColor;
    data->endColor = settings.endColor;
    data->emitCount = settings.emitCount;
    data->emitInterval = settings.emitInterval;
    data->currentTime = emitterTimers_[emitterId];
    data->emit = 0; // 後で更新
}

void ParticleCSGroup::ResetParticles() {
    InitializeParticles();
}

size_t ParticleCSGroup::GetActiveParticleCount() const {
    return activeParticleCount_;
}

std::string ParticleCSGroup::GetGroupName() const {
    return groupName_;
}

std::string ParticleCSGroup::GetModelPath() const {
    return modelPath_;
}

std::string ParticleCSGroup::GetTexturePath() const {
    return texturePath_;
}

PrimitiveType ParticleCSGroup::GetPrimitiveType() const {
    return primitiveType_;
}