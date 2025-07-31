#include "TitleScene.h"
#include <Frame.h>
#include"SkyBox/SkyBox.h"

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    obj_ = std::make_unique<BaseObject>();
    obj_->Init("walk");
    obj_->CreateModel("animation/sneakWalk.gltf");
    obj_->SetReflect(true);
    BaseObjectManager::GetInstance()->AddObject(std::move(obj_));

    SkyBox::GetInstance()->Initialize("game/skybox.dds");

    ptEditor_ = ParticleEditor::GetInstance();

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

    SkyBox::GetInstance()->Draw(vp_);

    BaseObjectManager::GetInstance()->Draw(vp_);

     ptEditor_->DrawAll(vp_);

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
    ptEditor_->EditorWindow();
    ptEditor_->DebugAll();
}

void TitleScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void TitleScene::ChangeScene() {
}