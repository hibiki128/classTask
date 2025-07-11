#include "Player.h"
#include "Particle/ParticleEditor.h"
#include <Input.h>
#include <cmath>

void Player::Init(const std::string objName) {
    BaseObject::Init(objName);
    BaseObject::CreateModel("animation/Idle.gltf");
    BaseObject::GetLoop() = true;

    // 初期値の設定
    velocity_ = {0.0f, 0.0f, 0.0f};
    isGrounded_ = true;
    isDashing_ = false;
    dashTimer_ = 0;
    groundY_ = 0.0f;

    particleEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("fire");
}

void Player::Update() {
    BaseObject::Update();
    Move();
    Jump();
    Dash();

    auto handPos = obj3d_->GetCurrentModelAnimation()->GetBone()->GetJointWorldPosition("mixamorig:RightHand",transform_->matWorld_);
    if (handPos.has_value()) {
        Vector3 worldHandPos = handPos.value();
        particleEmitter_->SetPosition(worldHandPos);
    }
    particleEmitter_->SetStartAcceX(-velocity_.x);
    particleEmitter_->SetStartAcceZ(-velocity_.z);
    particleEmitter_->Update();

    // 速度を位置に適用
    transform_->translation_.x += velocity_.x;
    transform_->translation_.y += velocity_.y;
    transform_->translation_.z += velocity_.z;

    // 地面判定
    if (transform_->translation_.y <= groundY_) {
        transform_->translation_.y = groundY_;
        velocity_.y = 0.0f;
        isGrounded_ = true;
    } else {
        isGrounded_ = false;
    }

    // 重力適用
    if (!isGrounded_) {
        velocity_.y -= gravity_;
    }

    // ダッシュ状態の更新（入力に基づく）
    isDashing_ = IsInputDash();

    // アニメーション状態の更新
    UpdateAnimation();
}

void Player::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection, offSet);
    particleEmitter_->Draw(viewProjection);
}

void Player::UpdateAnimation() {
    // ジャンプ中（空中にいる場合）
    if (!isGrounded_) {
        BaseObject::SetAnima("animation/Jump.gltf");
        return;
    }

    // 地上での動作
    bool isMoving = (velocity_.x != 0.0f || velocity_.z != 0.0f);

    if (isMoving) {
        if (isDashing_) {
            // ダッシュ中
            BaseObject::SetAnima("animation/Run.gltf");
        } else {
            // 通常移動
            BaseObject::SetAnima("animation/Jogging.gltf");
        }
    } else {
        // 静止状態
        BaseObject::SetAnima("animation/Idle.gltf");
    }
}

void Player::Move() {
    Vector3 moveDirection = {0.0f, 0.0f, 0.0f};

    // 入力取得
    bool left = IsInputLeft();
    bool right = IsInputRight();
    bool up = IsInputUp();
    bool down = IsInputDown();

    // 移動方向を設定
    if (left)
        moveDirection.x -= 1.0f;
    if (right)
        moveDirection.x += 1.0f;
    if (up)
        moveDirection.z += 1.0f;
    if (down)
        moveDirection.z -= 1.0f;

    // 移動入力がある場合
    if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
        // カメラの向きを取得
        float cameraYaw = 0.0f;
        if (camera_) {
            cameraYaw = camera_->GetYaw();
        }

        // カメラの向きに基づいて移動方向を回転
        float cos_yaw = std::cos(cameraYaw);
        float sin_yaw = std::sin(cameraYaw);

        float rotatedX = moveDirection.x * cos_yaw - moveDirection.z * sin_yaw;
        float rotatedZ = moveDirection.x * sin_yaw + moveDirection.z * cos_yaw;

        // 正規化
        float length = std::sqrt(rotatedX * rotatedX + rotatedZ * rotatedZ);
        if (length > 0.0f) {
            rotatedX /= length;
            rotatedZ /= length;
        }

        // 速度を設定
        float currentSpeed = isDashing_ ? dashSpeed_ : moveSpeed_;
        velocity_.x = rotatedX * currentSpeed;
        velocity_.z = rotatedZ * currentSpeed;

        // プレイヤーの向きを移動方向に設定
        transform_->rotation_.y = std::atan2(rotatedX, rotatedZ);
    } else {
        // 移動入力がない場合は水平方向の速度を0に
        velocity_.x = 0.0f;
        velocity_.z = 0.0f;
    }
}

void Player::Jump() {
    if (IsInputJump() && isGrounded_) {
        velocity_.y = jumpPower_;
        isGrounded_ = false;
    }
}

void Player::Dash() {
    // ダッシュ状態は入力に基づいて Update() で設定される
    // このメソッドは必要に応じて拡張できる
}

bool Player::IsInputLeft() {
    // キーボード入力
    if (Input::GetInstance()->PushKey(DIK_A))
        return true;

    // ゲームパッド入力
    XINPUT_STATE state;
    if (Input::GetInstance()->GetJoystickState(0, state)) {
        // 左スティック
        if (state.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            return true;
        // 十字キー
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
            return true;
    }

    return false;
}

bool Player::IsInputRight() {
    // キーボード入力
    if (Input::GetInstance()->PushKey(DIK_D))
        return true;

    // ゲームパッド入力
    XINPUT_STATE state;
    if (Input::GetInstance()->GetJoystickState(0, state)) {
        // 左スティック
        if (state.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            return true;
        // 十字キー
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
            return true;
    }

    return false;
}

bool Player::IsInputUp() {
    // キーボード入力
    if (Input::GetInstance()->PushKey(DIK_W))
        return true;

    // ゲームパッド入力
    XINPUT_STATE state;
    if (Input::GetInstance()->GetJoystickState(0, state)) {
        // 左スティック
        if (state.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            return true;
        // 十字キー
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
            return true;
    }

    return false;
}

bool Player::IsInputDown() {
    // キーボード入力
    if (Input::GetInstance()->PushKey(DIK_S))
        return true;

    // ゲームパッド入力
    XINPUT_STATE state;
    if (Input::GetInstance()->GetJoystickState(0, state)) {
        // 左スティック
        if (state.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            return true;
        // 十字キー
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
            return true;
    }

    return false;
}

bool Player::IsInputJump() {
    // キーボード入力
    if (Input::GetInstance()->TriggerKey(DIK_SPACE))
        return true;

    // ゲームパッド入力
    XINPUT_STATE state;
    XINPUT_STATE prevState;
    if (Input::GetInstance()->GetJoystickState(0, state) &&
        Input::GetInstance()->GetJoystickStatePrevious(0, prevState)) {
        // Aボタン（ジャンプ）
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) &&
            !(prevState.Gamepad.wButtons & XINPUT_GAMEPAD_A))
            return true;
    }

    return false;
}

bool Player::IsInputDash() {
    // キーボード入力
    if (Input::GetInstance()->PushKey(DIK_LSHIFT))
        return true;

    // ゲームパッド入力
    XINPUT_STATE state;
    if (Input::GetInstance()->GetJoystickState(0, state)) {
        // Bボタン（ダッシュ）
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
            return true;
    }

    return false;
}