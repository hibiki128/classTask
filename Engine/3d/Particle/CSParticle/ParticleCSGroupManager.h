#pragma once

#include "Externals/nlohmann/json.hpp"
#include "Particle/ParticleStruct.h"
#include "ParticleCSGroup.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ParticleCSGroupManager {
  private:
    /// ===================================================
    /// private method
    /// ===================================================
    static ParticleCSGroupManager *instance;
    ParticleCSGroupManager() = default;
    ~ParticleCSGroupManager() = default;
    ParticleCSGroupManager(const ParticleCSGroupManager &) = delete;
    ParticleCSGroupManager &operator=(const ParticleCSGroupManager &) = delete;

  public:
    /// ===================================================
    /// public method
    /// ===================================================
    static ParticleCSGroupManager *GetInstance();

    /// 初期化・終了処理
    void Initialize();
    void Finalize();

    /// グループ作成
    void CreateModelGroup(const std::string &groupName, const std::string &modelPath, const std::string &texturePath = "");
    void CreatePrimitiveGroup(const std::string &groupName, PrimitiveType primitiveType, const std::string &texturePath = "");

    /// グループ管理
    void RemoveGroup(const std::string &groupName);
    std::shared_ptr<ParticleCSGroup> GetGroup(const std::string &groupName);
    bool HasGroup(const std::string &groupName) const;
    std::vector<std::string> GetGroupNames() const;

    /// パーティクル制御
    void ResetAllParticles();
    size_t GetTotalParticleCount() const;

  private:
    /// JSON関連
    void LoadGroupFromJson(const std::string &filePath);
    void SaveGroupToJson(const std::string &groupName);

    /// ============================================
    /// private variables
    /// ============================================
    std::unordered_map<std::string, std::shared_ptr<ParticleCSGroup>> groups_;
    ParticleCSGroupManager *instance_ = nullptr;
};