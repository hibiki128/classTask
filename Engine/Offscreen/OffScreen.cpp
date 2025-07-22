#include "OffScreen.h"
#include "DirectXCommon.h"
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include <filesystem>
#include <fstream>
#include <iostream>

void OffScreen::Initialize() {
    dxCommon = DirectXCommon::GetInstance();
    psoManager_ = PipeLineManager::GetInstance();
    srvManager_ = SrvManager::GetInstance();

    CreateSmooth();
    CreateGauss();
    CreateVignette();
    CreateDepth();
    CreateRadial();
    CreateCinematic();
    CreatePingPongBuffers();
    CreateFinalResultTexture();
    InitializeDataHandler();
    LoadData();
}

void OffScreen::CreateFinalResultTexture() {
    // 最終結果用のレンダーターゲットリソースを作成
    finalResultResource_ = dxCommon->CreateRenderTextureResource(
        WinApp::kClientWidth,
        WinApp::kClientHeight,
        dxCommon->GetClearColorValue().Format,
        dxCommon->GetClearColorValue());

    // SRV作成（ImGui表示用）
    finalResultSrvIndex_ = srvManager_->Allocate();
    srvManager_->CreateSRVforRenderTexture(finalResultSrvIndex_, finalResultResource_.Get());
    finalResultSrvHandleCPU_ = srvManager_->GetCPUDescriptorHandle(finalResultSrvIndex_);
    finalResultSrvHandleGPU_ = srvManager_->GetGPUDescriptorHandle(finalResultSrvIndex_);

    // RTVハンドルを取得（ピンポンバッファの次の位置を使用）
    int rtvIndex = 3 + kPingPongBufferCount; // バックバッファ(0,1) + オフスクリーン(2) + ピンポンバッファ(3,4) の次

    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = dxCommon->GetRTVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
    UINT descriptorSize = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    finalResultRtvHandle_.ptr = rtvStartHandle.ptr + (rtvIndex * descriptorSize);

    // RTVを作成
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    dxCommon->GetDevice()->CreateRenderTargetView(finalResultResource_.Get(), &rtvDesc, finalResultRtvHandle_);
}

void OffScreen::InitializeDataHandler() {
    dataHandler_ = std::make_unique<DataHandler>("OffScreen", "OffScreenData");
}

void OffScreen::CreatePingPongBuffers() {
    // ピンポンバッファを作成
    for (int i = 0; i < kPingPongBufferCount; ++i) {
        // レンダーターゲットリソースを作成
        pingPongResources_[i] = dxCommon->CreateRenderTextureResource(WinApp::kClientWidth, WinApp::kClientHeight, dxCommon->GetClearColorValue().Format, dxCommon->GetClearColorValue());

        // SRV作成
        pingPongSrvIndices_[i] = srvManager_->Allocate();
        srvManager_->CreateSRVforRenderTexture(pingPongSrvIndices_[i], pingPongResources_[i].Get());
        pingPongSrvHandlesCPU_[i] = srvManager_->GetCPUDescriptorHandle(pingPongSrvIndices_[i]);
        pingPongSrvHandlesGPU_[i] = srvManager_->GetGPUDescriptorHandle(pingPongSrvIndices_[i]);

        // RTVハンドルを取得（DirectXCommonのRTVディスクリプタヒープから）
        // バックバッファ(0,1) + オフスクリーン(2) の後の位置を使用
        int rtvIndex = 3 + i; // オフスクリーン(2)の次から使用

        // RTVディスクリプタハンドルを取得
        D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = dxCommon->GetRTVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
        UINT descriptorSize = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        pingPongRtvHandles_[i].ptr = rtvStartHandle.ptr + (rtvIndex * descriptorSize);

        // RTVを作成
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        dxCommon->GetDevice()->CreateRenderTargetView(pingPongResources_[i].Get(), &rtvDesc, pingPongRtvHandles_[i]);
    }
}

void OffScreen::Draw() {
    if (effectChain_.empty()) {
        // エフェクトが無い場合
        DrawToFinalResult();
        CopyFinalResultToBackBuffer();
        return;
    }

    bool isFirstInput = true;
    int currentPingPongBuffer = 0;
    int outputBuffer = 0;

    // 有効なエフェクトのみを処理
    std::vector<int> enabledEffects;
    for (size_t i = 0; i < effectChain_.size(); ++i) {
        if (effectChain_[i].enabled) {
            enabledEffects.push_back(static_cast<int>(i));
        }
    }

    if (enabledEffects.empty()) {
        // 有効なエフェクトがない場合
        DrawToFinalResult();
        CopyFinalResultToBackBuffer();
        return;
    }

    // 有効なエフェクトチェーンを順番に適用
    for (size_t i = 0; i < enabledEffects.size(); ++i) {
        int effectIndex = enabledEffects[i];
        bool isLastEffect = (i == enabledEffects.size() - 1);

        if (isLastEffect) {
            // 最後のエフェクトは最終結果テクスチャに描画
            DrawSingleEffect(effectChain_[effectIndex].shaderMode, isFirstInput, currentPingPongBuffer, -2); // -2は最終結果を示す特別な値
        } else {
            // 中間エフェクトはピンポンバッファに描画
            DrawSingleEffect(effectChain_[effectIndex].shaderMode, isFirstInput, currentPingPongBuffer, outputBuffer);
            currentPingPongBuffer = outputBuffer;
            outputBuffer = 1 - outputBuffer;
            isFirstInput = false;
        }
    }

    // 最終結果をバックバッファにコピー
    CopyFinalResultToBackBuffer();
}

void OffScreen::DrawToFinalResult() {
    // 最終結果テクスチャに直接描画（エフェクトなし）
    dxCommon->GetCommandList()->OMSetRenderTargets(1, &finalResultRtvHandle_, false, nullptr);

    // バリア遷移
    dxCommon->BarrierTransition(finalResultResource_.Get(),
                                D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // レンダーターゲットをクリア
    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    dxCommon->GetCommandList()->ClearRenderTargetView(finalResultRtvHandle_, clearColor, 0, nullptr);

    // パイプライン設定
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, ShaderMode::kNone);

    // 入力テクスチャを設定
    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon->GetOffScreenGPUHandle());

    // 描画
    dxCommon->GetCommandList()->DrawInstanced(3, 1, 0, 0);

    // バリア遷移
    dxCommon->BarrierTransition(finalResultResource_.Get(),
                                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void OffScreen::CopyFinalResultToBackBuffer() {
    // バックバッファに最終結果をコピー
    UINT backBufferIndex = dxCommon->GetSwapChain()->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon->GetRTVCPUDescriptorHandle(backBufferIndex);
    dxCommon->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    // パイプライン設定
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, ShaderMode::kNone);

    // 最終結果テクスチャを入力として設定
    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, finalResultSrvHandleGPU_);

    // 描画
    dxCommon->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OffScreen::DrawSingleEffect(ShaderMode mode, bool isFirstInput, int inputPingPongIndex, int outputRtvIndex) {
    // 出力先を設定
    if (outputRtvIndex == -2) {
        // 最終結果テクスチャに描画
        dxCommon->GetCommandList()->OMSetRenderTargets(1, &finalResultRtvHandle_, false, nullptr);
        // バリア遷移
        dxCommon->BarrierTransition(finalResultResource_.Get(),
                                    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // レンダーターゲットをクリア
        const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        dxCommon->GetCommandList()->ClearRenderTargetView(finalResultRtvHandle_, clearColor, 0, nullptr);
    } else if (outputRtvIndex != -1) {
        // ピンポンバッファに描画
        dxCommon->GetCommandList()->OMSetRenderTargets(1, &pingPongRtvHandles_[outputRtvIndex], false, nullptr);
        // バリア遷移
        dxCommon->BarrierTransition(pingPongResources_[outputRtvIndex].Get(),
                                    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // レンダーターゲットをクリア
        const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        dxCommon->GetCommandList()->ClearRenderTargetView(pingPongRtvHandles_[outputRtvIndex], clearColor, 0, nullptr);
    } else {
        // バックバッファに描画（この分岐は使われなくなる）
        UINT backBufferIndex = dxCommon->GetSwapChain()->GetCurrentBackBufferIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon->GetRTVCPUDescriptorHandle(backBufferIndex);
        dxCommon->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    }

    // パイプライン設定とシェーダーパラメータ設定（既存のコードと同じ）
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, mode);

    // シェーダーモードに応じた定数バッファ設定（既存のコードと同じ）
    switch (mode) {
    case ShaderMode::kVigneet:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, vignetteResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kSmooth:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, smoothResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kGauss:
        dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, gaussianResouce->GetGPUVirtualAddress());
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
    }

    // 入力テクスチャを設定
    if (isFirstInput) {
        // 最初の入力はオフスクリーンバッファ
        dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon->GetOffScreenGPUHandle());
    } else {
        // 2回目以降はピンポンバッファ
        if (inputPingPongIndex >= 0 && inputPingPongIndex < kPingPongBufferCount) {
            dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, pingPongSrvHandlesGPU_[inputPingPongIndex]);
        } else {
            // エラーハンドリング - フォールバックとしてオフスクリーンバッファを使用
            dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon->GetOffScreenGPUHandle());
        }
    }

    // 描画
    dxCommon->GetCommandList()->DrawInstanced(3, 1, 0, 0);

    // バリア遷移
    if (outputRtvIndex == -2) {
        // 最終結果テクスチャの場合
        dxCommon->BarrierTransition(finalResultResource_.Get(),
                                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    } else if (outputRtvIndex != -1) {
        // ピンポンバッファの場合
        dxCommon->BarrierTransition(pingPongResources_[outputRtvIndex].Get(),
                                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    }
}

void OffScreen::Setting() {
#ifdef _DEBUG
    ImGui::Text("ポストエフェクト");

    // エフェクト追加ボタン
    const char *shaderModeItems[] = {"なし", "グレイ", "ビネット", "スムース", "ガウス", "アウトライン(エッジ検出)", "アウトライン(深度ベース)", "ブラー", "シネマティック"};
    static int selectedEffect = 0;

    ImGui::Combo("追加するエフェクト", &selectedEffect, shaderModeItems, IM_ARRAYSIZE(shaderModeItems));

    if (ImGui::Button("エフェクトを追加")) {
        AddEffect(static_cast<ShaderMode>(selectedEffect));
    }

    ImGui::Separator();

    // エフェクトチェーン表示・編集
    for (int i = 0; i < effectChain_.size(); ++i) {
        ImGui::PushID(i);

        ImGui::Text("エフェクト %d: %s", i + 1, shaderModeItems[static_cast<int>(effectChain_[i].shaderMode)]);

        ImGui::SameLine();
        if (ImGui::Checkbox("有効", &effectChain_[i].enabled)) {
            SetEffectEnabled(i, effectChain_[i].enabled);
        }

        ImGui::SameLine();
        if (ImGui::Button("上へ") && i > 0) {
            MoveEffectUp(i);
        }

        ImGui::SameLine();
        if (ImGui::Button("下へ") && i < effectChain_.size() - 1) {
            MoveEffectDown(i);
        }

        ImGui::SameLine();
        if (ImGui::Button("削除")) {
            RemoveEffect(i);
            ImGui::PopID();
            break; // インデックスが変わるのでbreak
        }

        // 各エフェクトの個別設定
        if (effectChain_[i].enabled) {
            switch (effectChain_[i].shaderMode) {
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
            }
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    if (ImGui::Button("セーブ")) {
        SaveData();
        std::string message = std::format("OffScreen saved.");
        MessageBoxA(nullptr, message.c_str(), "OffScreen", 0);
    }
#endif // _DEBUG
}

// エフェクト管理メソッド
void OffScreen::AddEffect(ShaderMode mode) {
    PostEffectSettings settings;
    settings.shaderMode = mode;
    settings.enabled = true;
    effectChain_.push_back(settings);
}

void OffScreen::RemoveEffect(int index) {
    if (index >= 0 && index < effectChain_.size()) {
        effectChain_.erase(effectChain_.begin() + index);
    }
}

void OffScreen::SetEffectEnabled(int index, bool enabled) {
    if (index >= 0 && index < effectChain_.size()) {
        effectChain_[index].enabled = enabled;
    }
}

void OffScreen::MoveEffectUp(int index) {
    if (index > 0 && index < effectChain_.size()) {
        std::swap(effectChain_[index], effectChain_[index - 1]);
    }
}

void OffScreen::MoveEffectDown(int index) {
    if (index >= 0 && index < effectChain_.size() - 1) {
        std::swap(effectChain_[index], effectChain_[index + 1]);
    }
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

// メインのセーブ関数
void OffScreen::SaveData() {
    SaveEffectChain();
    SaveEffectParameters();
}

// メインのロード関数
void OffScreen::LoadData() {
    LoadEffectChain();
    LoadEffectParameters();
}

// エフェクトチェーンの保存
void OffScreen::SaveEffectChain() {
    // エフェクト数を保存
    int effectCount = static_cast<int>(effectChain_.size());
    dataHandler_->Save<int>("effectCount", effectCount);

    // 各エフェクトの情報を保存
    for (int i = 0; i < effectCount; ++i) {
        std::string shaderModeKey = "effect" + std::to_string(i) + "_shaderMode";
        std::string enabledKey = "effect" + std::to_string(i) + "_enabled";

        dataHandler_->Save<int>(shaderModeKey, static_cast<int>(effectChain_[i].shaderMode));
        dataHandler_->Save<bool>(enabledKey, effectChain_[i].enabled);
    }
}

// エフェクトチェーンの読み込み
void OffScreen::LoadEffectChain() {
    // エフェクト数を読み込み
    int effectCount = dataHandler_->Load<int>("effectCount", 0);

    // エフェクトチェーンをクリア
    effectChain_.clear();

    // 各エフェクトを読み込み
    for (int i = 0; i < effectCount; ++i) {
        std::string shaderModeKey = "effect" + std::to_string(i) + "_shaderMode";
        std::string enabledKey = "effect" + std::to_string(i) + "_enabled";

        PostEffectSettings settings;
        settings.shaderMode = static_cast<ShaderMode>(dataHandler_->Load<int>(shaderModeKey, 0));
        settings.enabled = dataHandler_->Load<bool>(enabledKey, false);

        effectChain_.push_back(settings);
    }
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
}
