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
    CreateIndexResource();
    CreateVertexResource();
}

void ParticleCS::Draw(const ViewProjection& vp) {
    Update();

    perViewData->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData->billboardMatrix = vp.matView_;

    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    particleCommon_->DrawCommonSetting(BlendMode::kAdd);

    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, perViewResource->GetGPUVirtualAddress());
    srvManager_->SetGraphicsRootDescriptorTable(1, outputParticleSrvForVSIndex_);
    srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath_));
    commandList->SetGraphicsRootConstantBufferView(3, materialResource->GetGPUVirtualAddress());

    commandList->DrawIndexedInstanced(6, 1024, 0, 0,0);
}

void ParticleCS::Update() {
    particleCommon_->ComputeDrawCommonSetting();
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();

    // UAVとして使用するためのバリア
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = outputParticleResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);

    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle.second);
    commandList->Dispatch(1024, 1, 1);

    // SRVとして使用するためのバリア
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    commandList->ResourceBarrier(1, &barrier);
}

void ParticleCS::CreateOutputParticleResource() {
    outputParticleResource = dxCommon_->CreateBufferResource(sizeof(CSParticle) * 1024, true);

    // UAV用のインデックス（Compute Shader用）
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource.Get(), 1024, sizeof(CSParticle));

    // SRV用のインデックス（Vertex Shader用）
    outputParticleSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(outputParticleSrvForVSIndex_, outputParticleResource.Get(), 1024, sizeof(CSParticle));
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

void ParticleCS::CreateIndexResource() {
    // クアッド用のインデックス（2つの三角形で四角形）
    std::vector<uint32_t> indices = {
        0, 1, 2, // 最初の三角形
        2, 1, 3  // 二番目の三角形
    };

    indexResource = dxCommon_->CreateBufferResource(sizeof(uint32_t) * indices.size());
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * indices.size());
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    std::memcpy(indexData, indices.data(), sizeof(uint32_t) * indices.size());
}

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