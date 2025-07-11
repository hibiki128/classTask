#include "TitleScene.h"
#include <Frame.h>

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

   // BaseObjectManager::GetInstance()->LoadAll("CG4");
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    player_ = std::make_unique<Player>();
    player_->Init("Player");

    followCamera_ = std::make_unique<FollowCamera>();
    followCamera_->Init();
    followCamera_->SetTarget(player_->GetWorldTransform());

    player_->SetCamera(followCamera_.get());

    BaseObjectManager::GetInstance()->AddObject(std::move(player_));
}

void TitleScene::Finalize() {
    BaseScene::Finalize();
}

void TitleScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void TitleScene::Draw() {
    /// -------描画処理開始-------

    BaseObjectManager::GetInstance()->Draw(vp_);

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //-------------------------

    /// -------描画処理終了-------
}

void TitleScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void TitleScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void TitleScene::AddObjectSetting() {
}

void TitleScene::AddParticleSetting() {
}

void TitleScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        followCamera_->Update();
        vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
        vp_.matView_ = followCamera_->GetViewProjection().matView_;
        vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
    }
}

void TitleScene::ChangeScene() {
}