#pragma once
#include "ParticleManager.h"
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include <string>
#ifdef _DEBUG
#include "imgui.h"
#endif

#include "externals/nlohmann/json.hpp"

#include "Data/DataHandler.h"
#include <filesystem>
#include <fstream>

class ParticleEmitter {
  public:
    // コンストラクタでメンバ変数を初期化
    ParticleEmitter();

    void Initialize(std::string name = {});

    // 更新処理を行うUpdate関数
    void Update();

    void UpdateOnce();

    void Draw(const ViewProjection &vp_);

    void DrawEmitter();

    void Debug(); // ImGui用の関数を追加

    void AddParticleGroup(ParticleGroup *particleGroup);
    void RemoveParticleGroup(const std::string &name) {
        Manager_->RemoveParticleGroup(name);
    }

    int selectedGroupIndex_ = 0;

    void SetPosition(const std::string &groupName, const Vector3 &position) { particleSettings_[groupName].translate = position; }
    void SetPositionY(const std::string &groupName, float positionY) { particleSettings_[groupName].translate.y = positionY; }
    void SetRotate(const std::string &groupName, const Vector3 &rotate) { particleSettings_[groupName].rotation = rotate; }
    void SetRotateY(const std::string &groupName, float rotateY) { particleSettings_[groupName].rotation.y = rotateY; }
    void SetScale(const std::string &groupName, const Vector3 &scale) { particleSettings_[groupName].scale = scale; }
    void SetCount(const std::string &groupName, int count) { particleSettings_[groupName].count = count; }
    void SetStartRotate(const std::string &groupName, const Vector3 &startRotate) { particleSettings_[groupName].startRote = startRotate; }
    void SetEndRotate(const std::string &groupName, const Vector3 &endRotate) { particleSettings_[groupName].endRote = endRotate; }
    void SetActive(bool isActive) { isActive_ = isActive; }
    void SetFrequency(float frequency) { emitFrequency_ = frequency; }
    void SetName(const std::string &name) { name_ = name; }

  private:
    // パーティクルを発生させるEmit関数
    void Emit();
    void SaveToJson();
    void LoadFromJson();
    void LoadParticleGroup();
    ParticleSetting DefaultSetting();

    void DebugParticleData();

  private:
    using json = nlohmann::json;
    float elapsedTime_;   // 経過時間
    float emitFrequency_; // パーティクルの発生頻度

    bool isVisible_ = false;
    bool isBillBoard_ = false;
    bool isActive_ = false;
    bool isAuto_ = false;

    std::string name_;         // パーティクルの名前
    WorldTransform transform_; // 位置や回転などのトランスフォーム

    std::unordered_map<std::string, ParticleSetting> particleSettings_;

    std::unique_ptr<ParticleManager> Manager_;
    std::unique_ptr<DataHandler> datas_;
    std::vector<std::string> particleGroupNames_;
};