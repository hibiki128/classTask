#include "Player.h"
#include <Input.h>

void Player::Init(const std::string objName) {
    BaseObject::Init(objName);
    BaseObject::CreateModel("animation/walk.gltf");
    BaseObject::GetLoop() = true;
}

void Player::Update() {
    BaseObject::Update();
    Move();
}

void Player::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection,offSet);
}

void Player::Move() {
    if (Input::GetInstance()->PushKey(DIK_W)) {
        transform_->translation_.z += 0.1f;
    }
    if (Input::GetInstance()->PushKey(DIK_S)) {
        transform_->translation_.z -= 0.1f;
    }
    if (Input::GetInstance()->PushKey(DIK_A)) {
        transform_->translation_.x -= 0.1f;
    }
    if (Input::GetInstance()->PushKey(DIK_D)) {
        transform_->translation_.x += 0.1f;
    }
}