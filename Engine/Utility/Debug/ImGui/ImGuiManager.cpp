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
    //ID3D12DescriptorHeap *ppHeaps[] = {srvHeap_.Get()};
    //commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    // 描画コマンドを発行
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

        ImGui::EndMainMenuBar();
    }
}

void ImGuiManager::ShowHierarchyWindow() {
    ImGui::Begin("Hierarchy");

    ImGui::End();
}

void ImGuiManager::ShowInspectorWindow() {
    ImGui::Begin("Inspector");

    ImGui::End();
}

void ImGuiManager::ShowProjectWindow() {
    ImGui::Begin("Project");

    ImGui::End();
}

void ImGuiManager::ShowSceneWindow() {
    // ウィンドウの装飾（パディングやタイトルバー）を考慮
    ImVec2 padding = ImGui::GetStyle().WindowPadding;                                   // ウィンドウの左右パディング
    float titleBarHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2; // タイトルバーの高さ

    // ウィンドウサイズを設定
    ImVec2 windowSize = {sceneTextureSize_.x + padding.x * 2, sceneTextureSize_.y + padding.y * 2 + titleBarHeight};
    ImGui::SetNextWindowSize(windowSize);

    // シーンウィンドウ開始
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize);

    // レンダーテクスチャに描画する内容をウィンドウ内に描画する
    uint32_t srvIndex = dxCommon_->GetOffScreenSrvIndex();

    //
    ImGui::ImageWithBg(static_cast<ImTextureID>(SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex).ptr), sceneTextureSize_, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::End();
}

void ImGuiManager::ShowMainUI() {
    // メインタブ
    ShowMainMenu();
    // シーンウィンドウ
    ShowSceneWindow();
    // ヒエラルキーウィンドウ
    ShowHierarchyWindow();
    // インスペクターウィンドウ
    ShowInspectorWindow();
    // プロジェクトウィンドウを描画
    ShowProjectWindow();
}

bool &ImGuiManager::GetIsShowMainUI() {
    return isShowMainUI_;
}

#endif //_DEBUG