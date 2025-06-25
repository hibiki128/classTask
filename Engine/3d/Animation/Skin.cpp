#include "Skin.h"
#include "algorithm"
#include <DirectXCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <cassert>
#include <myMath.h>

void Skin::Initialize(const Skeleton &skeleton, const ModelData &modelData) {
    skinCluster_ = CreateSkinCluster(skeleton, modelData);
}

void Skin::Update(const Skeleton &skeleton) {
    for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
        assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
        skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
            skinCluster_.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
        skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
            Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
    }
}

SkinCluster Skin::CreateSkinCluster(const Skeleton &skeleton, const ModelData &modelData) {
    SkinCluster skinCluster;
    DirectXCommon *dxCommon = DirectXCommon::GetInstance();
    SrvManager *srvManager_ = SrvManager::GetInstance();

    // --- マルチメッシュ対応: 全頂点数を集計 ---

    for (const auto &mesh : modelData.meshes) {
        totalVertexCount += mesh.vertices.size();
    }

    // palette用のResourceを確保
    skinCluster.paletteResource = dxCommon->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
    WellForGPU *mappedPalette = nullptr;
    skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPalette));
    skinCluster.mappedPalette = {mappedPalette, skeleton.joints.size()};
    skinClusterPaletteSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.paletteSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterPaletteSrvIndex_);
    skinCluster.paletteSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterPaletteSrvIndex_);

    // palette用のSRVを作成
    srvManager_->CreateSRVforStructuredBuffer(skinClusterPaletteSrvIndex_, skinCluster.paletteResource.Get(), UINT(skeleton.joints.size()), sizeof(WellForGPU));

    // influence用のResourceを確保（全メッシュ分）
    skinCluster.influenceResource = dxCommon->CreateBufferResource(sizeof(VertexInfluence) * totalVertexCount);
    VertexInfluence *mappedInfluence = nullptr;
    skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedInfluence));
    std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * totalVertexCount);
    skinCluster.mappedInfluence = {mappedInfluence, totalVertexCount};
    skinClusterInfluenceSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.influenceSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterInfluenceSrvIndex_);
    skinCluster.influenceSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterInfluenceSrvIndex_);

    srvManager_->CreateSRVforStructuredBuffer(skinClusterInfluenceSrvIndex_, skinCluster.influenceResource.Get(), UINT(totalVertexCount), sizeof(VertexInfluence));

    // inputVertex用のResourceを確保
    skinCluster.inputVertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * totalVertexCount);
    VertexData *mappedVertex = nullptr;
    skinCluster.inputVertexResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedVertex));
    std::memset(mappedVertex, 0, sizeof(VertexData) * totalVertexCount);
    skinCluster.mappedVertex = {mappedVertex, totalVertexCount};
    skinClusterInputVertexSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.inputVertexSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterInputVertexSrvIndex_);
    skinCluster.inputVertexSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterInputVertexSrvIndex_);

    srvManager_->CreateSRVforStructuredBuffer(skinClusterInputVertexSrvIndex_, skinCluster.inputVertexResource.Get(), UINT(totalVertexCount), sizeof(VertexData));

    // outPutVertex用のResourceを確保
    skinCluster.outputVertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * totalVertexCount, true);
    skinClusterOutputVertexSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.outputVertexSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterOutputVertexSrvIndex_);
    skinCluster.outputVertexSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterOutputVertexSrvIndex_);

    srvManager_->CreateUAVStructuredBuffer(skinClusterOutputVertexSrvIndex_, skinCluster.outputVertexResource.Get(), static_cast<uint32_t>(totalVertexCount), sizeof(VertexData));

    skinCluster.outputVertexBufferView.BufferLocation = skinCluster.outputVertexResource->GetGPUVirtualAddress();
    skinCluster.outputVertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * totalVertexCount);
    skinCluster.outputVertexBufferView.StrideInBytes = sizeof(VertexData);

    // SkinningInformation用のResourceを確保
    skinCluster.skinningInformationResource = dxCommon->CreateBufferResource(sizeof(SkinningInformationForGPU));
    skinCluster.SkinningInfomationData = nullptr;
    skinCluster.skinningInformationResource->Map(0, nullptr, reinterpret_cast<void **>(&skinCluster.SkinningInfomationData));
    skinCluster.SkinningInfomationData->numVertices = static_cast<uint32_t>(totalVertexCount);

    // InverseBindPoseMatrixの保存領域を作成
    skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
    std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), []() { return MakeIdentity4x4(); });

    // --- マルチメッシュ・マルチマテリアル対応: 各メッシュごとにオフセットを管理 ---
    std::vector<size_t> meshVertexOffsets;
    meshVertexOffsets.reserve(modelData.meshes.size());
    size_t vertexOffset = 0;
    for (const auto &mesh : modelData.meshes) {
        meshVertexOffsets.push_back(vertexOffset);
        vertexOffset += mesh.vertices.size();
    }

    // ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める
    for (const auto &jointWeight : modelData.skinClusterData) {
        auto it = skeleton.jointMap.find(jointWeight.first);
        if (it == skeleton.jointMap.end()) {
            continue;
        }
        skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
        for (const auto &vertexWeight : jointWeight.second.vertexWeights) {
            // vertexWeight.meshIndex, vertexWeight.vertexIndex を想定
            size_t meshIndex = vertexWeight.meshIndex;
            size_t localVertexIndex = vertexWeight.vertexIndex;
            if (meshIndex >= meshVertexOffsets.size())
                continue;
            size_t globalVertexIndex = meshVertexOffsets[meshIndex] + localVertexIndex;
            if (globalVertexIndex >= totalVertexCount)
                continue;
            auto &currentInfluence = skinCluster.mappedInfluence[globalVertexIndex];
            for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
                if (currentInfluence.weights[index] == 0.0f) {
                    currentInfluence.weights[index] = vertexWeight.weight;
                    currentInfluence.jointIndices[index] = (*it).second;
                    break;
                }
            }
        }
    }

    return skinCluster;
}