#include "ParticleGroupManager.h"

ParticleGroupManager *ParticleGroupManager::instance = nullptr;

ParticleGroupManager *ParticleGroupManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleGroupManager();
    }
    return instance;
}

void ParticleGroupManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void ParticleGroupManager::AddParticleGroup(std::unique_ptr<ParticleGroup> particleGroup) {
    particleGroups_.emplace_back(std::move(particleGroup));
}
