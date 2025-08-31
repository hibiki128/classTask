#include "ParticleCSEmitter.h"
#include <Frame.h>
#include <Particle/ParticleCommon.h>

void ParticleCSEmitter::Initialize() {
    particleCommon_ = ParticleCommon::GetInstance();
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    commandList = dxCommon_->GetCommandList().Get();
    srvManager_ = SrvManager::GetInstance();
    CreateEmitterSphereResource();
}

void ParticleCSEmitter::Draw(const ViewProjection &vp) {
    for (auto &group : particleGroups_) {
        group->Update(vp);
        dxCommon_->TransitionUAVBarrier(group->GetOutputParticleResource().Get());
        EmitterDisPatch();
        group->UpdateParticleCSDisPatch();
        dxCommon_->TransitionSRVBarrier();
        particleCommon_->GPUDrawCommonSetting(group->GetParticleGroupData().blendMode);
        const auto &meshes = group->GetModelData().meshes;
        for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
            D3D12_INDEX_BUFFER_VIEW indexBufferView = group->GetIndexBufferView();
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = group->GetVertexBufferView();
            commandList->IASetIndexBuffer(&indexBufferView);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->SetGraphicsRootConstantBufferView(0, group->GetPerViewResource()->GetGPUVirtualAddress());
            srvManager_->SetGraphicsRootDescriptorTable(1, group->GetOutputParticleSrvForVSIndex());
            srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(group->GetParticleGroupData().materials[meshIndex].textureFilePath));
            commandList->SetGraphicsRootConstantBufferView(3, group->GetMaterialResource()->GetGPUVirtualAddress());
            commandList->DrawIndexedInstanced(UINT(meshes[meshIndex].indices.size()), kMaxParticleCount, 0, 0, 0);
        }
    }
}

void ParticleCSEmitter::Update() {
    if (isAuto_) {
        EmitterUpdate();
    }
}

void ParticleCSEmitter::DrawImGui() {
    ImGui::Begin("ParticleCS");
    int dragCount = int(emitterSphereData_->count);
    ImGui::DragInt("パーティクルの数", &dragCount, 1, 0, kMaxParticleCount);
    emitterSphereData_->count = uint32_t(dragCount);
    ImGui::DragFloat("発生間隔", &emitterSphereData_->frequency, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("エミッタの半径", &emitterSphereData_->radius, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("エミッタの座標", &emitterSphereData_->translate.x, 0.1f, -100.0f, 100.0f);
    ImGui::Checkbox("自動更新", &isAuto_);
    ImGui::End();
}

void ParticleCSEmitter::AddParticleGroup(ParticleCSGroup *particleGroup) {
    particleGroups_.push_back(particleGroup);
}

void ParticleCSEmitter::EmitterUpdate() {
    emitterSphereData_->frequencyTime += Frame::DeltaTime();
    if (emitterSphereData_->frequency <= emitterSphereData_->frequencyTime) {
        emitterSphereData_->frequencyTime -= emitterSphereData_->frequency;
        emitterSphereData_->emit = 1;
    } else {
        emitterSphereData_->emit = 0;
    }
}

void ParticleCSEmitter::CreateEmitterSphereResource() {
    emitterSphereResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSphere));
    emitterSphereResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterSphereData_));
    emitterSphereData_->count = 10;
    emitterSphereData_->frequency = 0.5f;
    emitterSphereData_->frequencyTime = 0.0f;
    emitterSphereData_->translate = Vector3(0.0f, 0.0f, 0.0f);
    emitterSphereData_->radius = 1.0f;
    emitterSphereData_->emit = 0;
}

void ParticleCSEmitter::EmitterDisPatch() {
    // EmitParticle.CSの処理
    particleCommon_->ComputeEmitterDrawCommonSetting();
    for (auto &group : particleGroups_) {
        commandList->SetComputeRootDescriptorTable(0, group->GetOutputParticleSrvHandle().second);
        commandList->SetComputeRootDescriptorTable(1, group->GetFreeListIndexSrvHandle().second);
        commandList->SetComputeRootDescriptorTable(2, group->GetFreeListSrvHandle().second);
        commandList->SetComputeRootConstantBufferView(3, emitterSphereResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(4, group->GetPerFrameResource()->GetGPUVirtualAddress());
        int disPatchCount = (emitterSphereData_->count + threadsPerGroup_ - 1) / threadsPerGroup_;
        commandList->Dispatch(disPatchCount, 1, 1);
    }
}