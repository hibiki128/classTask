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
    CreateOutputParticleResource();
    CreatePerViewResource();
    CreateMaterialResource();
    CreateIndexResource();
    CreateVertexResource();
    CreateEmitterSphereResource();
    CreatePerFrameResource();
    CreateFreeCounterResource();
    InitParticle();
}

void ParticleCS::Draw(const ViewProjection &vp) {
    EmitterUpdate();
    Update();

    perViewData_->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData_->billboardMatrix = vp.matView_;

    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    particleCommon_->DrawCommonSetting(BlendMode::kAdd);

    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
    srvManager_->SetGraphicsRootDescriptorTable(1, outputParticleSrvForVSIndex_);
    srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath_));
    commandList->SetGraphicsRootConstantBufferView(3, materialResource_->GetGPUVirtualAddress());

    commandList->DrawIndexedInstanced(6, 1024, 0, 0, 0);
}

void ParticleCS::InitParticle() {

    srvManager_->SetDescriptorHeap();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    particleCommon_->ComputeInitDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeCounterSrvHandle_.second);
    commandList->Dispatch(1024, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCS::EmitterUpdate() {
    emitterSphereData_->frequencyTime += Frame::DeltaTime();
    if (emitterSphereData_->frequency <= emitterSphereData_->frequencyTime) {
        emitterSphereData_->frequencyTime -= emitterSphereData_->frequency;
        emitterSphereData_->emit = 1;
    } else {
        emitterSphereData_->emit = 0;
    }
}

void ParticleCS::Update() {

    perFrameData_->time += Frame::DeltaTime();
    perFrameData_->deltaTime = Frame::DeltaTime();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    particleCommon_->ComputeEmitterDrawCommonSetting();

    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);                   // UAV (u0)
    commandList->SetComputeRootConstantBufferView(1, emitterSphereResource_->GetGPUVirtualAddress()); // CBV (b0)
    commandList->SetComputeRootConstantBufferView(2, perFrameResource_->GetGPUVirtualAddress());      // CBV (b0)
    commandList->SetComputeRootDescriptorTable(3, freeCounterSrvHandle_.second);

    commandList->Dispatch(1, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCS::CreateOutputParticleResource() {
    outputParticleResource_ = dxCommon_->CreateBufferResource(sizeof(CSParticle) * 1024, true);

    // UAV用のインデックス（Compute Shader用）
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource_.Get(), 1024, sizeof(CSParticle));

    // SRV用のインデックス（Vertex Shader用）
    outputParticleSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(outputParticleSrvForVSIndex_, outputParticleResource_.Get(), 1024, sizeof(CSParticle));
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

void ParticleCS::CreateEmitterSphereResource() {
    emitterSphereResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSphere));
    emitterSphereResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterSphereData_));
    emitterSphereData_->count = 10;
    emitterSphereData_->frequency = 0.5f;
    emitterSphereData_->frequencyTime = 0.0f;
    emitterSphereData_->translate = Vector3(0.0f, 0.0f, 0.0f);
    emitterSphereData_->radius = 1.0f;
    emitterSphereData_->emit = 0;
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

void ParticleCS::CreatePerFrameResource() {
    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void **>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = 0.0f;
}

void ParticleCS::CreateFreeCounterResource() {
    freeCounterResource_ = dxCommon_->CreateBufferResource(sizeof(int) * 1024, true);

    // UAV用のインデックス（Compute Shader用）
    freeCounterSrvIndex_ = srvManager_->Allocate() + 1;
    freeCounterSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeCounterSrvIndex_);
    freeCounterSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeCounterSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeCounterSrvIndex_, freeCounterResource_.Get(), 1024, sizeof(int));

    // SRV用のインデックス（Vertex Shader用）
    freeCounterSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(freeCounterSrvForVSIndex_, freeCounterResource_.Get(), 1024, sizeof(int));
}
