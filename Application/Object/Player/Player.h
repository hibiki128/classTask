#pragma once
#include "Camera/FollowCamera/FollowCamera.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"

class Player : public BaseObject {
  public:
    void Init(const std::string objName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    // カメラの設定
    void SetCamera(FollowCamera *camera) { camera_ = camera; }

  private:
    void Move();
    void Jump();
    void Dash();
    void UpdateAnimation(); // アニメーション更新メソッド

    // 入力取得関数
    bool IsInputLeft();
    bool IsInputRight();
    bool IsInputUp();
    bool IsInputDown();
    bool IsInputJump();
    bool IsInputDash();

  private:
    std::unique_ptr<ParticleEmitter> particleEmitter_;

    FollowCamera *camera_ = nullptr;

    // 移動関連
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    float moveSpeed_ = 0.1f;
    float dashSpeed_ = 0.25f;
    bool isDashing_ = false;
    int dashTimer_ = 0;
    int dashDuration_ = 20; // ダッシュの持続時間（フレーム）

    // ジャンプ関連
    bool isGrounded_ = true;
    float jumpPower_ = 0.8f;
    float gravity_ = 0.05f;
    float groundY_ = 0.0f; // 地面の高さ
};