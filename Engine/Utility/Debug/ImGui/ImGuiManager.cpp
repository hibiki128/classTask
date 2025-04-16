#include "ImGuiManager.h"
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_impl_win32.h"
#include <imgui_impl_dx12.h>

ImGuiManager *ImGuiManager::instance = nullptr;

void ImGuiManager::Initialize(WinApp *winApp) {

    dxCommon_ = DirectXCommon::GetInstance();

    // ImGuiのコンテキストを生成
    ImGui::CreateContext();

    // Docking機能を有効化
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Docking機能を有効化
    io.Fonts->Clear();                                // 既存のフォントをクリア

    io.Fonts->AddFontFromFileTTF("resources/fonts/PixelMplus12-Regular.ttf", 14.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

    // フォントの生成
    unsigned char *tex_pixels = nullptr;
    int tex_width, tex_height;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);
    // ImGuiのスタイルを設定
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(winApp->GetHwnd());

    // CreateDescriptorHeap();

    // srvインデックスを割り
    srvManager_ = SrvManager::GetInstance();

    uint32_t srvIndex = srvManager_->Allocate();
    ImGui_ImplDX12_Init(
        dxCommon_->GetDevice().Get(),
        2,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvManager_->GetDescriptorHeap(),
        srvManager_->GetCPUDescriptorHandle(srvIndex),
        srvManager_->GetGPUDescriptorHandle(srvIndex));
}

void ImGuiManager::CreateDescriptorHeap() {
    HRESULT result;

    // デスクリプタヒープ設定
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    // デスクリプタヒープ生成
    result = dxCommon_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));

    ImGui_ImplDX12_Init(
        dxCommon_->GetDevice().Get(),
        static_cast<int>(dxCommon_->GetBackBufferCount()),
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvHeap_.Get(),
        srvHeap_->GetCPUDescriptorHandleForHeapStart(),
        srvHeap_->GetGPUDescriptorHandleForHeapStart());
}

ImGuiManager *ImGuiManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ImGuiManager;
    }
    return instance;
}

void ImGuiManager::Finalize() {
    // 後始末
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // デスクリプタヒープを解放
    srvHeap_.Reset();

    delete instance;
    instance = nullptr;
}

void ImGuiManager::Begin() {
    // ImGuiフレーム開始
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::End() {
    // 描画前準備
    ImGui::Render();
}

void ImGuiManager::Draw() {
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();

    //// デスクリプタヒープの配列をセットするコマンド
    // ID3D12DescriptorHeap *ppHeaps[] = {srvHeap_.Get()};
    // commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    //  描画コマンドを発行
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiManager::ShowMainMenu() {
    // メインメニューバー作成
    if (ImGui::BeginMainMenuBar()) {

        // ファイルの処理
        if (ImGui::BeginMenu("ファイル")) {
            if (ImGui::MenuItem("追加")) { /* 新しいシーンの処理 */
            }
            if (ImGui::MenuItem("開く")) { /* シーンを開く処理 */
            }
            if (ImGui::MenuItem("セーブ")) { /* シーンを保存する処理 */
            }
            ImGui::Separator();
            if (ImGui::MenuItem("終了")) { /* アプリ終了処理 */
            }
            ImGui::EndMenu();
        }

        // オブジェクトの処理
        if (ImGui::BeginMenu("オブジェクト")) {

            // ゲームオブジェクト
            if (ImGui::BeginMenu("ゲームオブジェクト")) {
                ImGui::EndMenu();
            }

            // パーティクルグループ
            if (ImGui::BeginMenu("パーティクルグループ")) {
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // 表示切り替えメニュー
        if (ImGui::BeginMenu("表示モード")) {

            if (ImGui::MenuItem("フルスクリーン")) {
                WinApp::GetInstance()->ToggleFullScreen();
            }

            // isShowMainUI_がtrueなら「ゲームシーンへ」ボタンを表示
            if (isShowMainUI_) {
                if (ImGui::MenuItem("ゲームシーンへ")) {
                    isShowMainUI_ = false; // ゲームシーンに切り替え
                }
            }
            // falseなら「デバッグシーンへ」ボタンを表示
            else {
                if (ImGui::MenuItem("デバッグシーンへ")) {
                    isShowMainUI_ = true; // デバッグUIを表示
                    WinApp::GetInstance()->IsFullScreen() = false;
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void ImGuiManager::ShowSceneSettingWindow() {
    ImGui::Begin("シーン設定");
    currentScene_->AddSceneSetting();
    ImGui::End();
}

void ImGuiManager::ShowObjectSettingWindow() {
    ImGui::Begin("オブジェクト設定");
    currentScene_->AddObjectSetting();
    ImGui::End();
}

void ImGuiManager::ShowParticleSettingWindow() {
    ImGui::Begin("パーティクル設定");
    currentScene_->AddParticleSetting();
    ImGui::End();
}

void ImGuiManager::FixAspectRatio() {

    // 横幅ベースで16:9に合わせた高さ
    float adjustedHeight = (sceneTextureSize_.x * 9.0f / 16.0f);
    // 高さベースで16:9に合わせた横幅
    float adjustedWidth = (sceneTextureSize_.y * 16.0f / 9.0f);

    // 元のサイズとの差を計算
    float deltaFromWidth = std::abs(adjustedHeight - sceneTextureSize_.y);
    float deltaFromHeight = std::abs(adjustedWidth - sceneTextureSize_.x);

    // 近い方を採用
    if (deltaFromWidth < deltaFromHeight) {
        sceneTextureSize_.y = adjustedHeight;
    } else {
        sceneTextureSize_.x = adjustedWidth;
    }
}

void ImGuiManager::ShowSceneWindow() {
    // ImGuiウィンドウ開始前にNextWindowSizeは設定しない（手動サイズ変更を許可）
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    // ImGuiウィンドウの中の利用可能サイズを取得（スクロールバーやパディングを除いたサイズ）
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    // 利用可能サイズに収まるように16:9でsceneTextureSize_を補正する
    float maxWidth = contentRegion.x;
    float maxHeight = contentRegion.y;

    // 横幅ベースで16:9にしたときの高さ
    float adjustedHeight = maxWidth * 9.0f / 16.0f;
    // 高さベースで16:9にしたときの横幅
    float adjustedWidth = maxHeight * 16.0f / 9.0f;

    // 画面内に収まるように調整（できるだけ大きく保ちつつ、片方でオーバーしないように）
    if (adjustedHeight <= maxHeight) {
        sceneTextureSize_.x = maxWidth;
        sceneTextureSize_.y = adjustedHeight;
    } else {
        sceneTextureSize_.x = adjustedWidth;
        sceneTextureSize_.y = maxHeight;
    }

    // 背景カラー設定
    uint32_t srvIndex = dxCommon_->GetOffScreenSrvIndex();
    ImVec4 backgroundColor = ImVec4(dxCommon_->GetClearColor().x, dxCommon_->GetClearColor().y, dxCommon_->GetClearColor().z, dxCommon_->GetClearColor().w);

    // レンダーテクスチャをImGuiウィンドウに描画
    ImGui::ImageWithBg(static_cast<ImTextureID>(
                           SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex).ptr),
                       sceneTextureSize_, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                       backgroundColor);

    ImGui::End();
}



void ImGuiManager::ShowMainUI() {
    // シーンウィンドウ
    ShowSceneWindow();
    // ヒエラルキーウィンドウ
    ShowSceneSettingWindow();
    // インスペクターウィンドウ
    ShowObjectSettingWindow();
    // プロジェクトウィンドウを描画
    ShowParticleSettingWindow();
}

bool &ImGuiManager::GetIsShowMainUI() {
    return isShowMainUI_;
}

void ImGuiManager::ShowDockSpace() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    // DockSpaceの生成
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

#endif //_DEBUG