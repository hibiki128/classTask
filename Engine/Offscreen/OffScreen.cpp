#include "OffScreen.h"
#include "DirectXCommon.h"
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include <Graphics/Texture/TextureManager.h>
#include <filesystem>
#include <fstream>
#include <iostream>

void OffScreen::Initialize() {
    dxCommon = DirectXCommon::GetInstance();
    psoManager_ = PipeLineManager::GetInstance();
    srvManager_ = SrvManager::GetInstance();
    TextureManager::GetInstance()->LoadTexture(folderPath_);
    CreateSmooth();
    CreateGauss();
    CreateVignette();
    CreateDepth();
    CreateRadial();
    CreateCinematic();
    CreateDissolve();
    InitializeDataHandler();
    LoadData();
}

void OffScreen::InitializeDataHandler() {
    dataHandler_ = std::make_unique<DataHandler>("OffScreen", "OffScreenData");
}

void OffScreen::Draw() {
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, shaderMode_);
    // 選択されたShaderModeに基づいて描画設定を実行
    switch (shaderMode_) {
    case ShaderMode::kNone:
        break;
    case ShaderMode::kGray:
        break;
    case ShaderMode::kVigneet:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, vignetteResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kSmooth:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, smoothResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kGauss:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, gaussianResouce->GetGPUVirtualAddress());
        break;
    case ShaderMode::kOutLine:
        break;
    case ShaderMode::kDepth:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, depthResouce->GetGPUVirtualAddress());
        dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, dxCommon->GetDepthGPUHandle());
        break;
    case ShaderMode::kBlur:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, radialResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kCinematic:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, cinematicResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kDissolve:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, dissolveResource->GetGPUVirtualAddress());
        srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(folderPath_));
        break;
    default:
        break;
    }
    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon->GetOffScreenGPUHandle());
    dxCommon->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OffScreen::Setting() {
#ifdef _DEBUG

    // ShaderModeを文字列で表現
    const char *shaderModeItems[] = {"なし", "グレイ", "ビネット", "スムース", "ガウス", "アウトライン(エッジ検出)", "アウトライン(深度ベース)", "ブラー", "シネマティック", "ディゾルブ"};
    int currentShaderMode = static_cast<int>(shaderMode_);

    // Comboを描画してユーザーが選択した場合に値を更新
    if (ImGui::Combo("シェーダーモード", &currentShaderMode, shaderModeItems, IM_ARRAYSIZE(shaderModeItems))) {
        shaderMode_ = static_cast<ShaderMode>(currentShaderMode);
    }

    switch (shaderMode_) {
    case ShaderMode::kNone:
        break;
    case ShaderMode::kGray:
        break;
    case ShaderMode::kVigneet:
        ImGui::DragFloat("滑らかさ", &vignetteData->vignetteExponent, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("半径", &vignetteData->vignetteRadius, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("強度", &vignetteData->vignetteStrength, 0.01f);
        ImGui::DragFloat2("中心", &vignetteData->vignetteCenter.x, 0.01f, -10.0f, 10.0f);
        break;
    case ShaderMode::kSmooth:
        ImGui::DragInt("カーネルサイズ", &smoothData->kernelSize, 2, 3, 7);
        break;
    case ShaderMode::kGauss:
        ImGui::DragInt("カーネルサイズ", &gaussianData->kernelSize, 2, 3, 7);
        ImGui::DragFloat("シグマ", &gaussianData->sigma, 0.01f, 0.01f, 10.0f);
        break;
    case ShaderMode::kOutLine:
        break;
    case ShaderMode::kDepth:
        depthData->projectionInverse = Inverse(projectionInverse_);
        ImGui::DragInt("カーネルサイズ", &depthData->kernelSize, 2, 3, 7);
        break;
    case ShaderMode::kBlur:
        ImGui::DragFloat2("中心座標", &radialData->kCenter.x, 0.1f);
        ImGui::DragFloat("幅", &radialData->kBlurWidth, 0.01f);
        break;
    case ShaderMode::kCinematic:
        ImGui::DragFloat("コンストラクト", &cinematicData->contrast, 0.01f);
        ImGui::DragFloat("彩度", &cinematicData->saturation, 0.01f);
        ImGui::DragFloat("輝度", &cinematicData->brightness, 0.01f);
        break;
    case ShaderMode::kDissolve:
        ImGui::DragFloat("ディゾルブ値", &dissolveData->value, 0.01f, 0.0f, 1.0f);
        break;
    default:
        break;
    }
    if (ImGui::Button("セーブ")) {

        std::string message = std::format("OffScreen saved.");
        MessageBoxA(nullptr, message.c_str(), "OffScreen", 0);
    }
#endif // _DEBUG
}

void OffScreen::CreateSmooth() {
    smoothResource = dxCommon->CreateBufferResource(sizeof(KernelSettings));
    smoothResource->Map(0, nullptr, reinterpret_cast<void **>(&smoothData));
    smoothData->kernelSize = 3;
}

void OffScreen::CreateGauss() {
    gaussianResouce = dxCommon->CreateBufferResource(sizeof(GaussianParams));
    gaussianResouce->Map(0, nullptr, reinterpret_cast<void **>(&gaussianData));
    gaussianData->kernelSize = 3;
    gaussianData->sigma = 1;
}

void OffScreen::CreateVignette() {
    vignetteResource = dxCommon->CreateBufferResource(sizeof(VignetteParameter));
    vignetteResource->Map(0, nullptr, reinterpret_cast<void **>(&vignetteData));
    vignetteData->vignetteExponent = 1.0f;
    vignetteData->vignetteRadius = 1.0f;
    vignetteData->vignetteStrength = 1.0f;
    vignetteData->vignetteCenter = {0.5f, 0.5f};
}

void OffScreen::CreateDepth() {
    depthResouce = dxCommon->CreateBufferResource(sizeof(Depth));
    depthResouce->Map(0, nullptr, reinterpret_cast<void **>(&depthData));
    depthData->projectionInverse = MakeIdentity4x4();
    depthData->kernelSize = 3;
}

void OffScreen::CreateRadial() {
    radialResource = dxCommon->CreateBufferResource(sizeof(RadialBlur));
    radialResource->Map(0, nullptr, reinterpret_cast<void **>(&radialData));
    radialData->kBlurWidth = 0.01f;
    radialData->kCenter = {0.5f, 0.5f};
}

void OffScreen::CreateCinematic() {
    cinematicResource = dxCommon->CreateBufferResource(sizeof(Cinematic));
    cinematicResource->Map(0, nullptr, reinterpret_cast<void **>(&cinematicData));
    cinematicData->iResolution = {1280.0f, 720.0f};
    cinematicData->contrast = 1.05f;
    cinematicData->saturation = 0.68f;
    cinematicData->brightness = 0.13f;
}

void OffScreen::CreateDissolve() {
    dissolveResource = dxCommon->CreateBufferResource(sizeof(Dissolve));
    dissolveResource->Map(0, nullptr, reinterpret_cast<void **>(&dissolveData));
    dissolveData->value = 0.0f; // 初期値
}

// メインのセーブ関数
void OffScreen::SaveData() {
    SaveEffectParameters();
}

// メインのロード関数
void OffScreen::LoadData() {
    LoadEffectParameters();
}

// エフェクトパラメータの保存
void OffScreen::SaveEffectParameters() {
    // Vignette パラメータ
    if (vignetteData) {
        dataHandler_->Save<float>("vignette_exponent", vignetteData->vignetteExponent);
        dataHandler_->Save<float>("vignette_radius", vignetteData->vignetteRadius);
        dataHandler_->Save<float>("vignette_strength", vignetteData->vignetteStrength);
        dataHandler_->Save<Vector2>("vignette_center", vignetteData->vignetteCenter);
    }

    // Smooth パラメータ
    if (smoothData) {
        dataHandler_->Save<int>("smooth_kernelSize", smoothData->kernelSize);
    }

    // Gaussian パラメータ
    if (gaussianData) {
        dataHandler_->Save<int>("gaussian_kernelSize", gaussianData->kernelSize);
        dataHandler_->Save<float>("gaussian_sigma", gaussianData->sigma);
    }

    // Depth パラメータ
    if (depthData) {
        dataHandler_->Save<int>("depth_kernelSize", depthData->kernelSize);
    }

    // Radial Blur パラメータ
    if (radialData) {
        dataHandler_->Save<Vector2>("radial_center", radialData->kCenter);
        dataHandler_->Save<float>("radial_blurWidth", radialData->kBlurWidth);
    }

    // Cinematic パラメータ
    if (cinematicData) {
        dataHandler_->Save<Vector2>("cinematic_resolution", cinematicData->iResolution);
        dataHandler_->Save<float>("cinematic_contrast", cinematicData->contrast);
        dataHandler_->Save<float>("cinematic_saturation", cinematicData->saturation);
        dataHandler_->Save<float>("cinematic_brightness", cinematicData->brightness);
    }

    if (dissolveData) {
        dataHandler_->Save<float>("dissolve_value", dissolveData->value);
    }
}

// エフェクトパラメータの読み込み
void OffScreen::LoadEffectParameters() {
    // Vignette パラメータ
    if (vignetteData) {
        vignetteData->vignetteExponent = dataHandler_->Load<float>("vignette_exponent", 1.0f);
        vignetteData->vignetteRadius = dataHandler_->Load<float>("vignette_radius", 1.0f);
        vignetteData->vignetteStrength = dataHandler_->Load<float>("vignette_strength", 1.0f);
        vignetteData->vignetteCenter = dataHandler_->Load<Vector2>("vignette_center", {0.5f, 0.5f});
    }

    // Smooth パラメータ
    if (smoothData) {
        smoothData->kernelSize = dataHandler_->Load<int>("smooth_kernelSize", 3);
    }

    // Gaussian パラメータ
    if (gaussianData) {
        gaussianData->kernelSize = dataHandler_->Load<int>("gaussian_kernelSize", 3);
        gaussianData->sigma = dataHandler_->Load<float>("gaussian_sigma", 1.0f);
    }

    // Depth パラメータ
    if (depthData) {
        depthData->kernelSize = dataHandler_->Load<int>("depth_kernelSize", 3);
    }

    // Radial Blur パラメータ
    if (radialData) {
        radialData->kCenter = dataHandler_->Load<Vector2>("radial_center", {0.5f, 0.5f});
        radialData->kBlurWidth = dataHandler_->Load<float>("radial_blurWidth", 0.01f);
    }

    // Cinematic パラメータ
    if (cinematicData) {
        cinematicData->iResolution = dataHandler_->Load<Vector2>("cinematic_resolution", {1280.0f, 720.0f});
        cinematicData->contrast = dataHandler_->Load<float>("cinematic_contrast", 1.05f);
        cinematicData->saturation = dataHandler_->Load<float>("cinematic_saturation", 0.68f);
        cinematicData->brightness = dataHandler_->Load<float>("cinematic_brightness", 0.13f);
    }

    if (dissolveData) {
        dissolveData->value = dataHandler_->Load<float>("dissolve_value", 0.0f); // 初期値
    }
}
