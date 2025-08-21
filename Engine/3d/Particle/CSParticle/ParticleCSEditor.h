#pragma once

#include "Camera/ViewProjection/ViewProjection.h"
#include "Debug/ImGui/ImGuiManager.h"
#include "Model/ModelStructs.h"
#include "ParticleCSEmitter.h"
#include "ParticleCSGroupManager.h"
#include "Primitive/PrimitiveModel.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include"Particle/ParticleStruct.h"

// GPUパーティクルグループデータ
struct ParticleCSGroupData {
    // マテリアルデータ
    std::vector<MaterialData> materials;

    // パーティクルのリスト（GPU管理のため実際のデータはGPU側）
    std::vector<CSParticle> particles; // 参照用（実際のデータはGPU）

    // インスタンシングデータ用SRVインデックス
    uint32_t instancingSRVIndex = 0;

    // インスタンシングリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;

    // インスタンス数
    uint32_t instanceCount = 0;

    // インスタンシングデータを書き込むためのポインタ
    ParticleCSForGPU *instancingData = nullptr;

    // グループ名
    std::string groupName;

    // ブレンドモード
    BlendMode blendMode = BlendMode::kAdd;

    // モデルパス（モデルパーティクルの場合）
    std::string modelPath;

    // プリミティブタイプ（プリミティブパーティクルの場合）
    PrimitiveType primitiveType = PrimitiveType::None;

    // テクスチャパス
    std::string texturePath;

    // アクティブパーティクル数
    size_t activeParticleCount = 0;
};

class ParticleCSEditor {
  private:
    // シングルトンインスタンス
    static ParticleCSEditor *instance_;

    // プライベートコンストラクタ
    ParticleCSEditor() = default;

    // コピー禁止
    ParticleCSEditor(const ParticleCSEditor &) = delete;
    ParticleCSEditor &operator=(const ParticleCSEditor &) = delete;

    // パーティクルエミッター保持用マップ
    std::unordered_map<std::string, std::unique_ptr<ParticleCSEmitter>> emitters_;

    // 選択されたエミッターのインデックス
    int selectedEmitterIndex_ = 0;

    // 選択されたエミッターの名前
    std::string selectedEmitterName_;

    // パーティクルグループマネージャーポインタ
    ParticleCSGroupManager *groupManager_ = nullptr;

    // 統計情報
    std::unordered_map<std::string, ParticleCSStats> currentFrameStats_; // 現在フレームの統計
    std::unordered_map<std::string, ParticleCSStats> displayStats_;      // 表示用の統計（前フレーム確定分）
    uint64_t currentFrameNumber_ = 0;
    uint64_t lastUpdateFrame_ = 0;

    // ローカル変数（UIで使用）
    std::string localGroupName_;                             // パーティクルグループ名
    std::string localModelPath_;                             // モデルファイルパス
    std::string localTexturePath_;                           // テクスチャパス
    std::string localEmitterName_;                           // エミッター名
    PrimitiveType localPrimitiveType_ = PrimitiveType::None; // プリミティブタイプ

    // CollapsingHeaderの色を定義
    ImVec4 headerColors_[6];

    // ロード関連変数
    bool isLoad_ = false;
    bool statsCleared_ = false;
    bool statsDisplayedThisFrame_ = false;

    // カラーテーマの設定
    void SetupColors();

    // カラー付きCollapsingHeader表示関数
    bool ColoredCollapsingHeader(const char *label, int colorIndex);

    // モデル選択UI表示関数
    void ShowModelSelector();

    // グループ管理UI表示関数
    void ShowGroupManagement();

    // JSONファイル一覧取得関数
    std::vector<std::string> GetJsonFiles();

  public:
    // インスタンスの取得
    static ParticleCSEditor *GetInstance();

    // 終了処理
    static void Finalize();

    // 初期化
    void Initialize();

    // パーティクルエミッター追加（名前指定）
    void AddEmitter(const std::string &name);

    // パーティクルエミッター追加（名前・ファイル・テクスチャ指定）
    void AddEmitter(const std::string &name, const std::string &fileName, const std::string &texturePath);

    // パーティクルグループ作成（モデル使用）
    void CreateGroup(const std::string &groupName, const std::string &modelPath, const std::string &texturePath);

    // パーティクルグループ作成（プリミティブ使用）
    void CreatePrimitiveGroup(const std::string &groupName, PrimitiveType primitiveType, const std::string &texturePath);

    // 外部パーティクル数をセット（シーン側から呼び出し）
    void SetExternalParticleCount(const std::string &baseName, size_t count);

    // フレーム統計更新
    void UpdateFrameStats();

    // シーンパーティクル数表示
    void SceneParticleCount();

    // エミッターのテンプレートから新しいエミッターを作成
    std::unique_ptr<ParticleCSEmitter> CreateEmitterFromTemplate(const std::string &name);

    // ImGuiエディターウィンドウの表示
    void EditorWindow();

    // すべてのエミッターを描画
    void DrawAll(const ViewProjection &vp);

    // すべてのエミッターのデバッグ情報を表示
    void DebugAll();

    // ImGuiエディターの表示処理
    void ShowImGuiEditor();

    // JSONからのデータロード
    void LoadFromJson();

    // デストラクタ
    ~ParticleCSEditor() = default;
};