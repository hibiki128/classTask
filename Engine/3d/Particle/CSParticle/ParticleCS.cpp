#include "ParticleCS.h"
#include "myMath.h"
#include <Frame.h>
#include <type/Vector3.h>

void ParticleCS::Initialize() {
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    srvManager_ = SrvManager::GetInstance();
    particleCommon_ = ParticleCommon::GetInstance();
    texManager_ = TextureManager::GetInstance();
    texManager_->LoadTexture(texPath_);
    commandList = dxCommon_->GetCommandList().Get();
    primitiveModel_ = PrimitiveModel::GetInstance();

    // プリミティブの頂点数を設定
    primitiveVertexCounts_[PrimitiveType::Plane] = 6;
    primitiveVertexCounts_[PrimitiveType::Sphere] = 24;
    primitiveVertexCounts_[PrimitiveType::Cube] = 36;
    CreateOutputParticleResource();
    CreatePerViewResource();
    CreateMaterialResource();
    CreateIndexResource();
    CreateVertexResource();
    // CreateEmitterSphereResource();
    CreatePerFrameResource();
    CreateFreeListIndexResource();
    CreateFreeListResource();
    InitParticle();

    ParticleEmitterSettings defaultSettings;
    AddEmitter(defaultSettings);
}

void ParticleCS::Draw(const ViewProjection &vp) {
    EmitterUpdate();
    Update();

    perViewData_->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData_->billboardMatrix = vp.matView_;
    perViewData_->billboardMatrix.m[3][0] = 0.0f;
    perViewData_->billboardMatrix.m[3][1] = 0.0f;
    perViewData_->billboardMatrix.m[3][2] = 0.0f;
    perViewData_->billboardMatrix.m[3][3] = 1.0f;
    perViewData_->billboardMatrix = Inverse(perViewData_->billboardMatrix);

    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    particleCommon_->DrawCommonSetting(BlendMode::kAdd);

    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
    srvManager_->SetGraphicsRootDescriptorTable(1, outputParticleSrvForVSIndex_);
    srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath_));
    commandList->SetGraphicsRootConstantBufferView(3, materialResource_->GetGPUVirtualAddress());

    commandList->DrawIndexedInstanced(6, kMaxParticleCount, 0, 0, 0);
}

void ParticleCS::InitParticle() {
    srvManager_->SetDescriptorHeap();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    // InitParticle.CSの処理
    particleCommon_->ComputeInitDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    const uint32_t threadsPerGroup = 1024;
    uint32_t dispatchCount = (kMaxParticleCount + threadsPerGroup - 1) / threadsPerGroup;
    commandList->Dispatch(dispatchCount, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCS::EmitterUpdate() {
    float deltaTime = Frame::DeltaTime();

    for (uint32_t i = 0; i < emitterSettings_.size(); ++i) {
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

void ParticleCS::Update() {
    perFrameData_->time += Frame::DeltaTime();
    perFrameData_->deltaTime = Frame::DeltaTime();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    // 各エミッターに対してEmitParticle実行
    particleCommon_->ComputeEmitterDrawCommonSetting();

    for (uint32_t i = 0; i < emitterResources_.size(); ++i) {
        if (!emitterSettings_[i].isActive)
            continue;

        commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
        commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
        commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
        commandList->SetComputeRootConstantBufferView(3, emitterResources_[i]->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());

        uint32_t emitCount = emitterSettings_[i].emitCount;
        uint32_t threadGroupSize = 64; // numthreadsのxの数
        uint32_t dispatchCount = (emitCount + threadGroupSize - 1) / threadGroupSize;

        commandList->Dispatch(dispatchCount, 1, 1);
    }

    // UpdateParticle実行
    particleCommon_->ComputeUpdateEmitterDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    commandList->SetComputeRootConstantBufferView(3, perFrameResource_->GetGPUVirtualAddress());

    const uint32_t threadsPerGroup = 1024;
    uint32_t dispatchCount = (kMaxParticleCount + threadsPerGroup - 1) / threadsPerGroup;
    commandList->Dispatch(dispatchCount, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCS::CreateOutputParticleResource() {
    outputParticleResource_ = dxCommon_->CreateBufferResource(sizeof(CSParticle) * kMaxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource_.Get(), kMaxParticleCount, sizeof(CSParticle));

    // SRV用のインデックス（Vertex Shader用）
    outputParticleSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(outputParticleSrvForVSIndex_, outputParticleResource_.Get(), kMaxParticleCount, sizeof(CSParticle));
}

void ParticleCS::CreatePerViewResource() {
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerView));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void **>(&perViewData_));
    perViewData_->viewProjection = MakeIdentity4x4();
    perViewData_->billboardMatrix = MakeIdentity4x4();
}

void ParticleCS::CreateMaterialResource() {
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleMaterial));
    materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->uvTransform = MakeIdentity4x4();
}

void ParticleCS::CreateIndexResource() {
    // クアッド用のインデックス（2つの三角形で四角形）
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

// void ParticleCS::CreateEmitterSphereResource() {
//     emitterSphereResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSphere));
//     emitterSphereResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterSphereData_));
//     emitterSphereData_->count = 10;
//     emitterSphereData_->frequency = 0.5f;
//     emitterSphereData_->frequencyTime = 0.0f;
//     emitterSphereData_->translate = Vector3(0.0f, 0.0f, 0.0f);
//     emitterSphereData_->radius = 1.0f;
//     emitterSphereData_->emit = 0;
// }

void ParticleCS::CreateVertexResource() {
    // クアッド用の頂点データ
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
}

void ParticleCS::CreatePerFrameResource() {
    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void **>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = 0.0f;
}

void ParticleCS::CreateFreeListIndexResource() {
    freeListIndexResource_ = dxCommon_->CreateBufferResource(sizeof(int) * kMaxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    freeListIndexSrvIndex_ = srvManager_->Allocate() + 1;
    freeListIndexSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListIndexSrvIndex_);
    freeListIndexSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListIndexSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListIndexSrvIndex_, freeListIndexResource_.Get(), kMaxParticleCount, sizeof(int));

    // SRV用のインデックス（Vertex Shader用）
    freeListIndexSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(freeListIndexSrvForVSIndex_, freeListIndexResource_.Get(), kMaxParticleCount, sizeof(int));
}

void ParticleCS::CreateFreeListResource() {
    freeListResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * kMaxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    freeListSrvIndex_ = srvManager_->Allocate() + 1;
    freeListSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListSrvIndex_);
    freeListSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListSrvIndex_, freeListResource_.Get(), kMaxParticleCount, sizeof(uint32_t));

    // SRV用のインデックス（Vertex Shader用）
    freeListSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(freeListSrvForVSIndex_, freeListResource_.Get(), kMaxParticleCount, sizeof(uint32_t));
}

uint32_t ParticleCS::AddEmitter(const ParticleEmitterSettings &settings) {
    uint32_t emitterId = static_cast<uint32_t>(emitterSettings_.size());

    emitterSettings_.push_back(settings);
    emitterTimers_.push_back(0.0f);

    // GPU用リソース作成
    auto resource = dxCommon_->CreateBufferResource(sizeof(GPUEmitterData));
    emitterResources_.push_back(resource);

    GPUEmitterData *dataPtr;
    resource->Map(0, nullptr, reinterpret_cast<void **>(&dataPtr));
    emitterDataPtrs_.push_back(dataPtr);

    UpdateGPUEmitterData(emitterId);

    return emitterId;
}

void ParticleCS::RemoveEmitter(uint32_t emitterId) {
    if (emitterId >= emitterSettings_.size())
        return;

    emitterSettings_.erase(emitterSettings_.begin() + emitterId);
    emitterTimers_.erase(emitterTimers_.begin() + emitterId);
    emitterResources_.erase(emitterResources_.begin() + emitterId);
    emitterDataPtrs_.erase(emitterDataPtrs_.begin() + emitterId);
}

void ParticleCS::UpdateEmitterSettings(uint32_t emitterId, const ParticleEmitterSettings &settings) {
    if (emitterId >= emitterSettings_.size())
        return;

    emitterSettings_[emitterId] = settings;
    UpdateGPUEmitterData(emitterId);
}

void ParticleCS::UpdateGPUEmitterData(uint32_t emitterId) {
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

void ParticleCS::DrawImGui() {
    if (ImGui::Begin("パーティクルシステム")) {

        if (ImGui::CollapsingHeader("システム設定")) {
            ImGui::Text("最大パーティクル数: %d", kMaxParticleCount);
            ImGui::Text("アクティブパーティクル数: %u", activeParticleCount_);
        }

        // エミッター設定
        if (ImGui::CollapsingHeader("エミッター設定")) {
            for (uint32_t i = 0; i < emitterSettings_.size(); ++i) {
                ImGui::PushID(i);
                if (ImGui::TreeNode(("エミッター " + std::to_string(i)).c_str())) {

                    auto &settings = emitterSettings_[i];
                    bool changed = false;

                    changed |= ImGui::Checkbox("有効", &settings.isActive);
                    changed |= ImGui::DragFloat3("位置", &settings.position.x, 0.1f);
                    changed |= ImGui::DragFloat3("速度最小値", &settings.velocityMin.x, 0.1f);
                    changed |= ImGui::DragFloat3("速度最大値", &settings.velocityMax.x, 0.1f);
                    changed |= ImGui::DragFloat3("スケール最小値", &settings.scaleMin.x, 0.01f);
                    changed |= ImGui::DragFloat3("スケール最大値", &settings.scaleMax.x, 0.01f);
                    changed |= ImGui::DragFloat("寿命最小値", &settings.lifeTimeMin, 0.1f, 0.1f, 10.0f);
                    changed |= ImGui::DragFloat("寿命最大値", &settings.lifeTimeMax, 0.1f, 0.1f, 10.0f);

                    // 色設定
                    const char *colorModeItems[] = {"ランダム", "固定色", "寿命補間"};
                    int currentColorMode = static_cast<int>(settings.colorMode);
                    if (ImGui::Combo("色モード", &currentColorMode, colorModeItems, 3)) {
                        settings.colorMode = static_cast<ColorMode>(currentColorMode);
                        changed = true;
                    }

                    if (settings.colorMode == ColorMode::Fixed || settings.colorMode == ColorMode::LifeTimeLerp) {
                        changed |= ImGui::ColorEdit4("開始色", &settings.startColor.x);
                        if (settings.colorMode == ColorMode::LifeTimeLerp) {
                            changed |= ImGui::ColorEdit4("終了色", &settings.endColor.x);
                        }
                    }

                    // エミッション設定
                    int emitCount = static_cast<int>(settings.emitCount);
                    if (ImGui::SliderInt("一回の発生数", &emitCount, 1, 10000)) {
                        settings.emitCount = static_cast<uint32_t>(emitCount);
                        changed = true;
                    }
                    changed |= ImGui::DragFloat("発生間隔", &settings.emitInterval, 0.001f, 0.001f, 5.0f);

                    // プリミティブタイプ
                    const char *primitiveItems[] = {"平面", "球", "立方体", "円柱", "リング", "三角形", "円錐", "ピラミッド"};
                    int currentPrimitive = static_cast<int>(settings.primitiveType) - 1; // Noneを除く
                    if (ImGui::Combo("プリミティブ", &currentPrimitive, primitiveItems, 8)) {
                        settings.primitiveType = static_cast<PrimitiveType>(currentPrimitive + 1);
                        changed = true;
                    }

                    if (changed) {
                        UpdateGPUEmitterData(i);
                    }

                    if (ImGui::Button("削除")) {
                        RemoveEmitter(i);
                        ImGui::TreePop();
                        ImGui::PopID();
                        break;
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (ImGui::Button("エミッター追加")) {
                ParticleEmitterSettings newSettings;
                AddEmitter(newSettings);
            }
        }
    }
    ImGui::End();
}