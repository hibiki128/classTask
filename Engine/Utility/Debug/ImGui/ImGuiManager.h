#pragma once
#include "DirectXCommon.h"
#include"Srv/SrvManager.h"
#include "WinApp.h"
class ImGuiManager {
  private:
    /// ====================================
    /// public method
    /// ====================================

    static ImGuiManager *instance;

    ImGuiManager() = default;
    ~ImGuiManager() = default;
    ImGuiManager(ImGuiManager &) = delete;
    ImGuiManager &operator=(ImGuiManager &) = delete;

  public:
    /// ====================================
    /// public method
    /// ====================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(WinApp *winApp);

    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns></returns>
    static ImGuiManager *GetInstance();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// ImGui受付開始
    /// </summary>
    void Begin();

    /// <summary>
    /// ImGui受付終了
    /// </summary>
    void End();

    /// <summary>
    /// 画面への描画
    /// </summary>
    void Draw();

    void ShowMainUI();

    bool &GetIsShowMainUI();

  private:
    /// ====================================
    /// private method
    /// ====================================

    /// <summary>
    /// ヒープ作成
    /// </summary>
    void CreateDescriptorHeap();

    /// <summary>
    /// メニュー表示
    /// </summary>
    void ShowMainMenu();

    /// <summary>
    /// シーン表示
    /// </summary>
    void ShowSceneWindow();

    /// <summary>
    /// ヒエラルキー表示
    /// </summary>
    void ShowHierarchyWindow();

    void ShowInspectorWindow();

    void ShowProjectWindow();

  private:
    /// ====================================
    /// private variaus
    /// ====================================

    // SRV用デスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    SrvManager *srvManager_ = nullptr;

    DirectXCommon *dxCommon_;

    // ヒエラルキーウィンドウ
    ImVec2 hierarchyWindowPosition_ = {0.0f, 64.0f};

    // シーンウィンドウ
    ImVec2 sceneTextureSize_ = {256.0f, 72.0f};

    // エンジンのウィンドウを描画するフラグ
    bool isShowMainUI_ = false;
};
