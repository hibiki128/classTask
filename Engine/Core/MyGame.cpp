#include "MyGame.h"
#include "SceneFactory.h"

void MyGame::Initialize() {
    Framework::Initialize();
    Framework::LoadResource();
    Framework::PlaySounds();
    // -----ゲーム固有の処理-----

    // 最初のシーンの生成
    sceneFactory_ = new SceneFactory();
    // シーンマネージャに最初のシーンをセット
    sceneManager_->SetSceneFactory(sceneFactory_);
    sceneManager_->NextSceneReservation("TITLE");
    // -----------------------
}

void MyGame::Finalize() {
    // -----ゲーム固有の処理-----

    // -----------------------

    Framework::Finalize();
}

void MyGame::Update() {
    Framework::Update();
    // -----ゲーム固有の処理-----

    // エンジンUI描画切り替え
    if (input->TriggerKey(DIK_O)) {
        imGuiManager_->GetIsShowMainUI() = !imGuiManager_->GetIsShowMainUI();
    }

    // -----------------------
}

void MyGame::Draw() {
    dxCommon->PreRenderTexture();
    srvManager->PreDraw();
    // -----描画開始-----

    // -----シーンごとの処理------

    object3dCommon->DrawCommonSetting();
    if (sceneManager_->GetTransitionEnd()) {
        collisionManager_->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    }
    sceneManager_->Draw();

#ifdef _DEBUG
    //-----線描画-----
    DrawLine3D::GetInstance()->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    //---------------
#endif // _DEBUG

    dxCommon->PreDraw();

    offscreen_->SetProjection(sceneManager_->GetBaseScene()->GetViewProjection()->matProjection_);
    if (imGuiManager_->GetIsShowMainUI()) {
        imGuiManager_->ShowMainUI();
    } else {
        offscreen_->Draw();
    }

    dxCommon->TransitionDepthBarrier();
    sceneManager_->DrawForOffScreen();
    sceneManager_->DrawTransition();

#ifdef _DEBUG
    Framework::DisplayFPS();
    imGuiManager_->End();
    imGuiManager_->Draw();
#endif // _DEBUG
       // ------------------------

    // -----描画終了-----
    dxCommon->PostDraw();
}