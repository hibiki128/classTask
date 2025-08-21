#pragma once

#include "ParticleCSGroup.h"
#include "ParticleCSGroupManager.h"
#include "Transform/WorldTransform.h"
#include "type/Matrix4x4.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <Data/DataHandler.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

#ifdef _DEBUG
#include "imgui.h"
#endif

#include"Particle/ParticleStruct.h"

class ParticleCSEmitter {
  public:
    /// ===================================================
    /// public method
    /// ===================================================
    ParticleCSEmitter();
    ~ParticleCSEmitter();

    /// 初期化・更新
    void Initialize(const std::string &name = "");
    void Update();
    void UpdateOnce();

    /// 描画
    void Draw(const ViewProjection &vp);
    void DrawEmitter();

    /// エミッション制御
    void Emit();

    /// グループ管理
    void AttachGroup(const std::string &groupName);
    void DetachGroup(const std::string &groupName);

    /// デバッグ・設定
    void Debug();
    void LoadFromJson();
    void SaveToJson();

    /// ユーティリティ
    std::unique_ptr<ParticleCSEmitter> Clone() const;

    /// ゲッター・セッター
    const std::string &GetName() const { return name_; }
    void SetName(const std::string &name) { name_ = name; }

    bool IsActive() const { return isActive_; }
    void SetActive(bool isActive) { isActive_ = isActive; }

    bool IsVisible() const { return isVisible_; }
    void SetVisible(bool isVisible) { isVisible_ = isVisible; }

    bool IsAuto() const { return isAuto_; }
    void SetAuto(bool isAuto) { isAuto_ = isAuto; }

    float GetEmitFrequency() const { return emitFrequency_; }
    void SetEmitFrequency(float frequency) { emitFrequency_ = frequency; }

    uint32_t GetEmitCount() const { return emitCount_; }
    void SetEmitCount(uint32_t count) { emitCount_ = count; }

    // トランスフォーム関連
    void SetPosition(const Vector3 &position) { transform_.translation_ = position; }
    Vector3 GetPosition() const { return transform_.translation_; }

    void SetScale(const Vector3 &scale) { transform_.scale_ = scale; }
    Vector3 GetScale() const { return transform_.scale_; }

    void SetRotation(const Quaternion &rotation) { transform_.quateRotation_ = rotation; }
    Quaternion GetRotation() const { return transform_.quateRotation_; }

    // パーティクル設定
    void SetVelocityRange(const Vector3 &min, const Vector3 &max) {
        velocityMin_ = min;
        velocityMax_ = max;
    }

    void SetScaleRange(const Vector3 &min, const Vector3 &max) {
        scaleMin_ = min;
        scaleMax_ = max;
    }

    void SetLifeTimeRange(float min, float max) {
        lifeTimeMin_ = min;
        lifeTimeMax_ = max;
    }

    void SetColorMode(ParticleColorMode mode) { colorMode_ = mode; }
    void SetStartColor(const Vector4 &color) { startColor_ = color; }
    void SetEndColor(const Vector4 &color) { endColor_ = color; }

    // アタッチされたグループの取得
    const std::vector<std::string> &GetAttachedGroups() const { return attachedGroups_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================
    void UpdateEmitterSettings(ParticleCSGroup *group);
    void UpdateStatistics();

    /// ===================================================
    /// private variables
    /// ===================================================

    // 基本設定
    std::string name_;
    WorldTransform transform_;
    bool isActive_;
    bool isVisible_;
    bool isAuto_;

    // エミッション設定
    float elapsedTime_;
    float emitFrequency_;
    uint32_t emitCount_;

    // パーティクル設定
    Vector3 velocityMin_;
    Vector3 velocityMax_;
    Vector3 scaleMin_;
    Vector3 scaleMax_;
    float lifeTimeMin_;
    float lifeTimeMax_;

    // カラー設定
    ParticleColorMode colorMode_;
    Vector4 startColor_;
    Vector4 endColor_;

    // グループ管理
    std::vector<std::string> attachedGroups_;
    ParticleCSGroupManager *groupManager_;

    // データ管理
    std::unique_ptr<DataHandler> datas_;
};