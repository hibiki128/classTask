#include "ParticleCS.h"
#include "myMath.h"

void ParticleCS::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    srvManager_ = SrvManager::GetInstance();
    particleCommon_ = ParticleCommon::GetInstance();
    texManager_ = TextureManager::GetInstance();
    texManager_->LoadTexture(texPath_);
    CreateOutputParticleResource();
    CreatePerViewResource();
    CreateMaterialResource();
}

void ParticleCS::Draw(const ViewProjection& vp) {
    Update();

    perViewData->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData->billboardMatrix = vp.matView_;

    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    particleCommon_->DrawCommonSetting(BlendMode::kAdd);

    commandList->IASetIndexBuffer(nullptr);
    commandList->IASetVertexBuffers(0, 1, &outputParticleBufferView);
    commandList->SetGraphicsRootConstantBufferView(0, perViewResource->GetGPUVirtualAddress());
    srvManager_->SetGraphicsRootDescriptorTable(1, outputParticleSrvIndex_);
    srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath_));
    commandList->SetGraphicsRootConstantBufferView(3, materialResource->GetGPUVirtualAddress());

    commandList->DrawInstanced(6, 1024, 0, 0);
}

void ParticleCS::Update() {
    particleCommon_->ComputeDrawCommonSetting();
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = outputParticleResource.Get();
    commandList->ResourceBarrier(1, &barrier);

    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle.second);

    commandList->Dispatch(1024, 1, 1);

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = outputParticleResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    commandList->ResourceBarrier(1, &barrier);
}

void ParticleCS::CreateOutputParticleResource() {

    outputParticleResource = dxCommon_->CreateBufferResource(sizeof(CSParticle) * 1024, true);
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);

    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource.Get(), 1024, sizeof(CSParticle));

    outputParticleBufferView.BufferLocation = outputParticleResource->GetGPUVirtualAddress();
    outputParticleBufferView.SizeInBytes = UINT(sizeof(CSParticle) * 1024);
    outputParticleBufferView.StrideInBytes = sizeof(CSParticle);
}

void ParticleCS::CreatePerViewResource() {
    perViewResource = dxCommon_->CreateBufferResource(sizeof(PerView));
    perViewResource->Map(0, nullptr, reinterpret_cast<void **>(&perViewData));
    perViewData->viewProjection = MakeIdentity4x4();
    perViewData->billboardMatrix = MakeIdentity4x4();
}

void ParticleCS::CreateMaterialResource() {
    materialResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(ParticleMaterial));
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData->uvTransform = MakeIdentity4x4();
}
