#include "TitleScene.h"
#include "SceneManager.h"
#include <Engine/Frame/Frame.h>

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    objCommon_ = Object3dCommon::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    ParticleEditor::GetInstance()->AddParticleEmitter("hit");
    emitter_ = std::make_unique<ParticleEmitter>();
    emitter_ = ParticleEditor::GetInstance()->GetEmitter("hit");

    test = std::make_unique<BaseObject>();
    test->Init("test");
    test->CreatePrimitiveModel(PrimitiveType::Ring);
    test->SetTexture("debug/uvChecker.png");

    ease_.time = 0.0f;
    ease_.maxTime = 1.0f;
    ease_.amplitude = 0.3f;
    ease_.period = 0.2f;
}

void TitleScene::Finalize() {
}

void TitleScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    ease_.time += Frame::DeltaTime();
    if (ease_.time >= ease_.maxTime - 0.3f) {
        ease_.time = 0.0f;
    }
    test->GetWorldScale() = EaseAmplitudeScale<Vector3>({1.0f, 1.0f, 1.0f}, ease_.time, ease_.maxTime, ease_.amplitude, ease_.period);

    test->Update();
}

void TitleScene::Draw() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //-------------------------

    objCommon_->DrawCommonSetting();
    //-----3DObjectの描画開始-----
    test->Draw(vp_);
    //--------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------
    emitter_->Draw(vp_);
    //-----------------------------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void TitleScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    objCommon_->DrawCommonSetting();
    //-----3DObjectの描画開始-----

    //--------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void TitleScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void TitleScene::AddObjectSetting() {
    if (ImGui::BeginTabBar("イージング設定")) {
        if (ImGui::BeginTabItem("イージング設定")) {
            ImGui::DragFloat("最大時間", &ease_.maxTime, 0.1f);
            ImGui::DragFloat("イージング時間", &ease_.time, 0.1f);
            ImGui::DragFloat("振幅", &ease_.amplitude, 0.1f);
            ImGui::DragFloat("周期", &ease_.period, 0.1f);
            ImGui::EndTabItem();
        };
        ImGui::EndTabBar();
    }
    test->DebugImGui();
}

void TitleScene::AddParticleSetting() {
    emitter_->Debug();
}

void TitleScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void TitleScene::ChangeScene() {

#ifdef _DEBUG

#endif // _DEBUG
#ifndef _DEBUG
    if (input_->TriggerKey(DIK_SPACE)) {
        sceneManager_->NextSceneReservation("GAME");
    }
#endif // !_DEBUG
}