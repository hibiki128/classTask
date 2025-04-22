#pragma once

#include"memory"
#include"ParticleGroup.h"

class ParticleGroupManager {
  private:
    /// ===================================================
    /// private method
    /// ===================================================
    static ParticleGroupManager *instance;
    ParticleGroupManager() = default;
    ~ParticleGroupManager() = default;
    ParticleGroupManager(ParticleGroupManager &) = delete;
    ParticleGroupManager &operator=(ParticleGroupManager &) = delete;

  public:
    /// ===================================================
    /// public method
    /// ===================================================
    static ParticleGroupManager *GetInstance();

    void Finalize();

    void AddParticleGroup(std::unique_ptr<ParticleGroup>particleGroup);

    std::vector<ParticleGroup>& GetParticleGroups() {
        std::vector<ParticleGroup> groups;
        for (const auto &group : particleGroups_) {
            groups.push_back(*group);
        }
        return groups;
    }
  private:
    /// ============================================
    /// private variaus
    /// ============================================
    
    std::vector<std::unique_ptr<ParticleGroup>> particleGroups_;
};
