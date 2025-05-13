#pragma once
#include "DirectXCommon.h"
#include "Srv/SrvManager.h"
#include "WinApp.h"
#include <BaseScene.h>
#include <application/Base/BaseObject.h>

#include "Object/BaseObjectManager.h"


class ImGuizmoManager;
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

    ImGuizmoManager *imGuizmoManager_ = nullptr;

  public:
    /// ====================================
    /// public method
    /// ====================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(WinApp *winApp);

    void SetupTheme();

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

    /// <summary>
    /// メインUI表示
    /// </summary>
    void ShowMainUI();

    /// <summary>
    /// メニュー表示
    /// </summary>
    void ShowMainMenu();

    /// <summary>
    /// ドックスペース追加
    /// </summary>
    void ShowDockSpace();

    bool &GetIsShowMainUI();
    void SetCurrentScene(BaseScene *currentScene) { currentScene_ = currentScene; };

    void SetImGuizmoManager(ImGuizmoManager *manager) {
        imGuizmoManager_ = manager;
    }

    // 必要に応じてImGuizmoManagerへのアクセサを追加
    ImGuizmoManager *GetImGuizmoManager() const {
        return imGuizmoManager_;
    }

  private:
    /// ====================================
    /// private method
    /// ====================================

    /// <summary>
    /// ヒープ作成
    /// </summary>
    void CreateDescriptorHeap();

    /// <summary>
    /// シーン表示
    /// </summary>
    void ShowSceneWindow();

    /// <summary>
    /// ヒエラルキー表示
    /// </summary>
    void ShowSceneSettingWindow();

    void ShowObjectSettingWindow();

    void ShowParticleSettingWindow();

    void FixAspectRatio();

  private:
    /// ====================================
    /// private variaus
    /// ====================================

    // SRV用デスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    SrvManager *srvManager_ = nullptr;
    BaseScene *currentScene_ = nullptr;

    DirectXCommon *dxCommon_;

    // ヒエラルキーウィンドウ
    ImVec2 hierarchyWindowPosition_ = {0.0f, 64.0f};

    // シーンウィンドウ
    ImVec2 sceneTextureSize_ = {800.0f, 450.0f};

    int cubeCount = 0;
    int sphereCount = 0;
    int planeCount = 0;
    int cylinderCount = 0;
    int ringCount = 0;
    int triangleCount = 0;
    int capsuleCount = 0;
    int pyramidCount = 0;
    int coneCount = 0;

    // エンジンのウィンドウを描画するフラグ
    bool isShowMainUI_ = false;
    // 重いUIコンポーネントの表示状態管理
    bool showSceneView_ = true;
    bool showObjectView_ = true;
    bool showParticleView_ = true;

    BaseObjectManager *baseObjectManager_ = nullptr;
};
