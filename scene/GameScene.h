#pragma once
#include "BaseScene.h"

class GameScene : public BaseScene {
  public: // メンバ関数
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw() override;

    /// <summary>
    /// オフスクリーン上に描画
    /// </summary>
    void DrawForOffScreen() override;

    ViewProjection *GetViewProjection() override { return &vp_; }

  private:
    void Debug();

    void CameraUpdate();

    void ChangeScene();

  private:
    Audio *audio_;
    Input *input_;
    Object3dCommon *objCommon_;
    SpriteCommon *spCommon_;
    ParticleCommon *ptCommon_;

    ViewProjection vp_;
    std::unique_ptr<DebugCamera> debugCamera_;
};
