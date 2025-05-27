#include "ParticleManager.h"
#include "Engine/Frame/Frame.h"
#include "Texture/TextureManager.h"
#include <fstream>
#include <random>

void ParticleManager::Initialize(SrvManager *srvManager) {
    particleCommon = ParticleCommon::GetInstance();
    srvManager_ = srvManager;
    randomEngine.seed(seedGenerator());
}

void ParticleManager::Update(const ViewProjection &viewProjection) {
    Matrix4x4 viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;
    Matrix4x4 billboardMatrix = viewProjection.matView_;
    billboardMatrix.m[3][0] = 0.0f;
    billboardMatrix.m[3][1] = 0.0f;
    billboardMatrix.m[3][2] = 0.0f;
    billboardMatrix.m[3][3] = 1.0f;
    billboardMatrix = Inverse(billboardMatrix);

    for (auto &[groupName, particleGroup] : particleGroups_) {
        uint32_t numInstance = 0;
        ParticleSetting &particleSetting = particleSettings_[groupName];

        for (auto particleIterator = particleGroup->GetParticleGroupData().particles.begin();
             particleIterator != particleGroup->GetParticleGroupData().particles.end();) {

            if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {
                particleIterator = particleGroup->GetParticleGroupData().particles.erase(particleIterator);
                continue;
            }
            float t = (*particleIterator).currentTime / (*particleIterator).lifeTime;
            t = std::clamp(t, 0.0f, 1.0f);

            if (particleSetting.isSinMove) {
                float waveScale = 0.5f * (sin(t * DirectX::XM_PI * 18.0f) + 1.0f);
                float maxScale = (1.0f - t);
                (*particleIterator).transform.scale_ =
                    (*particleIterator).startScale * waveScale * maxScale;
            } else {
                (*particleIterator).transform.scale_ =
                    (1.0f - t) * (*particleIterator).startScale + t * (*particleIterator).endScale;
                if (!(particleSetting.isGatherMode && t >= particleSetting.gatherStartRatio)) {
                    (*particleIterator).color.w = (*particleIterator).initialAlpha - ((*particleIterator).currentTime / (*particleIterator).lifeTime);
                }
            }

            bool isGathering = false;
            if (particleSetting.isGatherMode && t >= particleSetting.gatherStartRatio) {
                isGathering = true;
                float gatherFactor = (t - particleSetting.gatherStartRatio) / (1.0f - particleSetting.gatherStartRatio);
                gatherFactor = std::clamp(gatherFactor, 0.0f, 1.0f);
                Vector3 toEmitter = (*particleIterator).emitterPosition - (*particleIterator).transform.translation_;
                float distance = toEmitter.Length();
                float distanceBasedAlpha = distance / (distance + 0.5f);
                (*particleIterator).color.w = (*particleIterator).initialAlpha * (1.0f - gatherFactor) * distanceBasedAlpha;
                if (distance < 0.05f) {
                    (*particleIterator).currentTime = (*particleIterator).lifeTime;
                    ++particleIterator;
                    continue;
                }
                float distanceFactor = std::min(1.0f, distance);
                toEmitter = toEmitter.Normalize();
                float gatherSpeed = particleSetting.gatherStrength * gatherFactor * distanceFactor * 3.0f;
                Vector3 gatherVelocity = toEmitter * gatherSpeed * Frame::DeltaTime();
                (*particleIterator).velocity = gatherVelocity;
                (*particleIterator).transform.translation_ += (*particleIterator).velocity;
            }

            if (!isGathering) {
                (*particleIterator).Acce = (1.0f - t) * (*particleIterator).startAcce + t * (*particleIterator).endAcce;
                if (particleSetting.isFaceDirection) {
                    Vector3 forward = (*particleIterator).fixedDirection;
                    Vector3 initialUp = {0.0f, 1.0f, 0.0f};
                    Vector3 rotationAxis = initialUp.Cross(forward).Normalize();
                    float dotProduct = initialUp.Dot(forward);
                    float angle = acosf(std::clamp(dotProduct, -1.0f, 1.0f));
                    (*particleIterator).transform.rotation_.x = rotationAxis.x * angle;
                    (*particleIterator).transform.rotation_.y = rotationAxis.y * angle;
                    (*particleIterator).transform.rotation_.z = rotationAxis.z * angle;
                } else if (particleSetting.isRandomRotate) {
                    (*particleIterator).transform.rotation_ += (*particleIterator).rotateVelocity;
                } else {
                    (*particleIterator).transform.rotation_ =
                        (1.0f - t) * (*particleIterator).startRote + t * (*particleIterator).endRote;
                }
                if (particleSetting.isAcceMultiply) {
                    (*particleIterator).velocity *= (*particleIterator).Acce;
                } else {
                    (*particleIterator).velocity += (*particleIterator).Acce;
                }
                (*particleIterator).transform.translation_ +=
                    (*particleIterator).velocity * Frame::DeltaTime();
            }
            (*particleIterator).currentTime += Frame::DeltaTime();

            Matrix4x4 worldMatrix{};
            if (particleSetting.isBillboard) {
                worldMatrix = MakeScaleMatrix((*particleIterator).transform.scale_) * billboardMatrix *
                              MakeTranslateMatrix((*particleIterator).transform.translation_);
            } else {
                worldMatrix = MakeAffineMatrix((*particleIterator).transform.scale_,
                                               (*particleIterator).transform.rotation_,
                                               (*particleIterator).transform.translation_);
            }
            Matrix4x4 worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;
            if (numInstance < particleGroup->GetMaxInstance()) {
                particleGroup->GetParticleGroupData().instancingData[numInstance].WVP = worldViewProjectionMatrix;
                particleGroup->GetParticleGroupData().instancingData[numInstance].World = worldMatrix;
                particleGroup->GetParticleGroupData().instancingData[numInstance].color = (*particleIterator).color;
                particleGroup->GetParticleGroupData().instancingData[numInstance].color.w = (*particleIterator).color.w;
                ++numInstance;
            }
            ++particleIterator;
        }
        particleGroup->GetParticleGroupData().instanceCount = numInstance;
    }
}

void ParticleManager::Draw() {
    for (auto &[groupName, particleGroup] : particleGroups_) {
        const auto &meshes = particleGroup->GetModelData().meshes;
        for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
            D3D12_INDEX_BUFFER_VIEW indexBufferView = particleGroup->GetIndexBufferView();
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = particleGroup->GetVertexBufferView();
            particleCommon->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
            particleCommon->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
            if (particleGroup->GetParticleGroupData().instanceCount > 0) {
                particleCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, particleGroup->GetmaterialResource()->GetGPUVirtualAddress());
                srvManager_->SetGraphicsRootDescriptorTable(1, particleGroup->GetParticleGroupData().instancingSRVIndex);
                srvManager_->SetGraphicsRootDescriptorTable(2, particleGroup->GetParticleGroupData().materials[meshIndex].textureIndex);
                particleCommon->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(
                    UINT(meshes[meshIndex].indices.size()),
                    particleGroup->GetParticleGroupData().instanceCount,
                    0, 0, 0);
            }
        }
    }
}

void ParticleManager::AddParticleGroup(ParticleGroup *particleGroup) {
    assert(particleGroup);
    std::string groupName = particleGroup->GetGroupName();
    particleGroups_.insert(std::pair(groupName, particleGroup));
    particleGroupNames_.push_back(groupName);
    // デフォルト設定を追加
    if (particleSettings_.find(groupName) == particleSettings_.end()) {
        particleSettings_[groupName] = ParticleSetting{};
    }
}

void ParticleManager::RemoveParticleGroup(const std::string &name) {
    // マップから削除
    particleGroups_.erase(name);
    particleSettings_.erase(name);

    // vector からも削除
    auto it = std::find(particleGroupNames_.begin(), particleGroupNames_.end(), name);
    if (it != particleGroupNames_.end()) {
        particleGroupNames_.erase(it);
    }
}

void ParticleManager::SetParticleSetting(const std::string &groupName, const ParticleSetting &setting) {
    particleSettings_[groupName] = setting;
}
ParticleSetting &ParticleManager::GetParticleSetting(const std::string &groupName) {
    return particleSettings_[groupName];
}
std::vector<std::string> ParticleManager::GetParticleGroupsName() {
    return particleGroupNames_;
}

Particle ParticleManager::MakeNewParticle(std::mt19937 &randomEngine, const ParticleSetting &setting) {
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distVelocityX(setting.velocityMin.x, setting.velocityMax.x);
    std::uniform_real_distribution<float> distVelocityY(setting.velocityMin.y, setting.velocityMax.y);
    std::uniform_real_distribution<float> distVelocityZ(setting.velocityMin.z, setting.velocityMax.z);
    std::uniform_real_distribution<float> distLifeTime(setting.lifeTimeMin, setting.lifeTimeMax);
    std::uniform_real_distribution<float> distAlpha(setting.alphaMin, setting.alphaMax);

    Particle particle;
    Vector3 randomTranslate;
    particle.emitterPosition = setting.translate;
    if (setting.isEmitOnEdge) {
        std::uniform_int_distribution<int> edgeSelector(0, 11);
        std::uniform_real_distribution<float> edgePosition(0.0f, 1.0f);
        int selectedEdge = edgeSelector(randomEngine);
        float position = edgePosition(randomEngine);
        const Vector3 v0 = {-1.0f, -1.0f, -1.0f};
        const Vector3 v1 = {1.0f, -1.0f, -1.0f};
        const Vector3 v2 = {-1.0f, 1.0f, -1.0f};
        const Vector3 v3 = {1.0f, 1.0f, -1.0f};
        const Vector3 v4 = {-1.0f, -1.0f, 1.0f};
        const Vector3 v5 = {1.0f, -1.0f, 1.0f};
        const Vector3 v6 = {-1.0f, 1.0f, 1.0f};
        const Vector3 v7 = {1.0f, 1.0f, 1.0f};
        const std::pair<Vector3, Vector3> edges[] = {
            {v0, v1}, {v1, v3}, {v3, v2}, {v2, v0}, {v4, v5}, {v5, v7}, {v7, v6}, {v6, v4}, {v0, v4}, {v1, v5}, {v2, v6}, {v3, v7}};
        const Vector3 &start = edges[selectedEdge].first;
        const Vector3 &end = edges[selectedEdge].second;
        randomTranslate = {
            start.x + (end.x - start.x) * position,
            start.y + (end.y - start.y) * position,
            start.z + (end.z - start.z) * position};
        randomTranslate.x *= setting.scale.x;
        randomTranslate.y *= setting.scale.y;
        randomTranslate.z *= setting.scale.z;
    } else {
        randomTranslate = {
            distribution(randomEngine) * setting.scale.x,
            distribution(randomEngine) * setting.scale.y,
            distribution(randomEngine) * setting.scale.z};
    }
    Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(setting.rotation);
    Vector3 rotatedPosition = {
        randomTranslate.x * rotationMatrix.m[0][0] + randomTranslate.y * rotationMatrix.m[1][0] + randomTranslate.z * rotationMatrix.m[2][0],
        randomTranslate.x * rotationMatrix.m[0][1] + randomTranslate.y * rotationMatrix.m[1][1] + randomTranslate.z * rotationMatrix.m[2][1],
        randomTranslate.x * rotationMatrix.m[0][2] + randomTranslate.y * rotationMatrix.m[1][2] + randomTranslate.z * rotationMatrix.m[2][2]};
    particle.transform.translation_ = setting.translate + rotatedPosition;

    if (setting.isRandomAllSize) {
        std::uniform_real_distribution<float> distScaleX(setting.allScaleMin.x, setting.allScaleMax.x);
        std::uniform_real_distribution<float> distScaleY(setting.allScaleMin.y, setting.allScaleMax.y);
        std::uniform_real_distribution<float> distScaleZ(setting.allScaleMin.z, setting.allScaleMax.z);
        particle.startScale = {distScaleX(randomEngine), distScaleY(randomEngine), distScaleZ(randomEngine)};
        if (setting.isEndScale) {
            particle.endScale = particle.startScale;
        }
    } else if (setting.isRandomSize) {
        std::uniform_real_distribution<float> distScale(setting.scaleMin, setting.scaleMax);
        particle.startScale.x = distScale(randomEngine);
        particle.startScale.y = particle.startScale.x;
        particle.startScale.z = particle.startScale.x;
    } else {
        particle.startScale = setting.particleStartScale;
    }
    if (!setting.isEndScale) {
        particle.endScale = setting.particleEndScale;
    }
    particle.startAcce = setting.startAcce;
    particle.endAcce = setting.endAcce;
    Vector3 randomVelocity = {
        distVelocityX(randomEngine),
        distVelocityY(randomEngine),
        distVelocityZ(randomEngine)};
    particle.velocity = {
        randomVelocity.x * rotationMatrix.m[0][0] + randomVelocity.y * rotationMatrix.m[1][0] + randomVelocity.z * rotationMatrix.m[2][0],
        randomVelocity.x * rotationMatrix.m[0][1] + randomVelocity.y * rotationMatrix.m[1][1] + randomVelocity.z * rotationMatrix.m[2][1],
        randomVelocity.x * rotationMatrix.m[0][2] + randomVelocity.y * rotationMatrix.m[1][2] + randomVelocity.z * rotationMatrix.m[2][2]};
    if (setting.isRandomRotate) {
        std::uniform_real_distribution<float> distRotateX(setting.rotateStartMin.x, setting.rotateStartMax.x);
        std::uniform_real_distribution<float> distRotateY(setting.rotateStartMin.y, setting.rotateStartMax.y);
        std::uniform_real_distribution<float> distRotateZ(setting.rotateStartMin.z, setting.rotateStartMax.z);
        particle.transform.rotation_.x = distRotateX(randomEngine);
        particle.transform.rotation_.y = distRotateY(randomEngine);
        particle.transform.rotation_.z = distRotateZ(randomEngine);
        if (setting.isRotateVelocity) {
            std::uniform_real_distribution<float> distRotateXVelocity(setting.rotateVelocityMin.x, setting.rotateVelocityMax.x);
            std::uniform_real_distribution<float> distRotateYVelocity(setting.rotateVelocityMin.y, setting.rotateVelocityMax.y);
            std::uniform_real_distribution<float> distRotateZVelocity(setting.rotateVelocityMin.z, setting.rotateVelocityMax.z);
            particle.rotateVelocity.x = distRotateXVelocity(randomEngine);
            particle.rotateVelocity.y = distRotateYVelocity(randomEngine);
            particle.rotateVelocity.z = distRotateZVelocity(randomEngine);
        }
    } else {
        particle.startRote = setting.startRote;
        particle.endRote = setting.endRote;
    }
    if (setting.isRandomColor) {
        std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
        particle.color = {distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), distAlpha(randomEngine)};
    } else {
        particle.color = {1.0f, 1.0f, 1.0f, distAlpha(randomEngine)};
    }
    if (setting.isFaceDirection) {
        Vector3 initialUp = {0.0f, 1.0f, 0.0f};
        Vector3 forward = particle.velocity.Normalize();
        particle.fixedDirection = forward;
        Vector3 rotationAxis = initialUp.Cross(forward).Normalize();
        float dotProduct = initialUp.Dot(forward);
        float angle = acosf(std::clamp(dotProduct, -1.0f, 1.0f));
        particle.transform.rotation_.x = rotationAxis.x * angle;
        particle.transform.rotation_.y = rotationAxis.y * angle;
        particle.transform.rotation_.z = rotationAxis.z * angle;
    }
    particle.initialAlpha = distAlpha(randomEngine);
    particle.lifeTime = distLifeTime(randomEngine);
    particle.currentTime = 0.0f;
    return particle;
}

std::list<Particle> ParticleManager::Emit() {
    std::list<Particle> allNewParticles;
    for (auto &[groupName, particleGroup] : particleGroups_) {
        std::list<Particle> newParticles;
        ParticleSetting &setting = particleSettings_[groupName];
        for (uint32_t nowCount = 0; nowCount < setting.count; ++nowCount) {
            Particle particle = MakeNewParticle(randomEngine, setting);
            newParticles.push_back(particle);
        }
        particleGroup->GetParticleGroupData().particles.splice(
            particleGroup->GetParticleGroupData().particles.end(),
            newParticles);
        allNewParticles.splice(allNewParticles.end(), newParticles);
    }
    return allNewParticles;
}
