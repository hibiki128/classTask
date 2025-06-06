#define NOMINMAX
#include "ParticleEditor.h"
#include "ImGui/ImGuiManager.h"
#ifdef _DEBUG
#include"ShowFolder/ShowFolder.h"
#endif // _DEBUG

ParticleEditor *ParticleEditor::instance = nullptr;

ParticleEditor *ParticleEditor::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleEditor();
    }
    return instance;
}

void ParticleEditor::Finalize() {
    delete instance;
    instance = nullptr;
}

void ParticleEditor::Initialize() {
    particleGroupManager_ = ParticleGroupManager::GetInstance();
    // カラーテーマの初期設定
    SetupColors();
}

// カラーテーマの設定メソッドを追加
void ParticleEditor::SetupColors() {
    // 各CollapsingHeaderに使用する色を定義
    headerColors_[0] = ImVec4(0.2f, 0.4f, 0.8f, 0.8f); // 青系
    headerColors_[1] = ImVec4(0.8f, 0.4f, 0.2f, 0.8f); // オレンジ系
    headerColors_[2] = ImVec4(0.2f, 0.7f, 0.4f, 0.8f); // 緑系
    headerColors_[3] = ImVec4(0.7f, 0.3f, 0.7f, 0.8f); // 紫系
    headerColors_[4] = ImVec4(0.7f, 0.7f, 0.2f, 0.8f); // 黄色系
    headerColors_[5] = ImVec4(0.5f, 0.5f, 0.5f, 0.8f); // グレー系
}

void ParticleEditor::AddParticleEmitter(const std::string &name, const std::string &fileName, const std::string &texturePath) {
    // 新しい ParticleEmitter を作成
    auto emitter = std::make_unique<ParticleEmitter>();

    // 初期化処理
    emitter->Initialize(name);
    // マップに追加
    emitters_[name] = std::move(emitter);
}

void ParticleEditor::Load() {
}

void ParticleEditor::AddParticleEmitter(const std::string &name) {
    // 新しい ParticleEmitter を作成
    auto emitter = std::make_unique<ParticleEmitter>();

    // 初期化処理
    emitter->Initialize(name);
    // マップに追加
    emitters_[name] = std::move(emitter);
}

void ParticleEditor::AddParticleGroup(const std::string &name, const std::string &fileName, const std::string &texturePath) {
    // 新しい ParticleGroup を作成
    auto group = std::make_unique<ParticleGroup>();
    // 初期化処理
    group->Initialize();
    // パーティクルグループを作成
    group->CreateParticleGroup(name, fileName, texturePath);

    // マップに追加
    particleGroupManager_->AddParticleGroup(std::move(group));
}

void ParticleEditor::AddPrimitiveParticleGroup(const std::string &name, const std::string &texturePath, PrimitiveType type) {
    // 新しい ParticleGroup を作成
    auto group = std::make_unique<ParticleGroup>();
    // 初期化処理
    group->Initialize();
    // パーティクルグループを作成
    group->CreatePrimitiveParticleGroup(name, type, texturePath);

    // マップに追加
    particleGroupManager_->AddParticleGroup(std::move(group));
}

void ParticleEditor::DrawAll(const ViewProjection &vp_) {
    for (auto &[name, emitter] : emitters_) {
        if (emitter) {
            if (emitter->GetIsAuto()) {
                emitter->Update();
            }
            emitter->Draw(vp_);
        }
    }
}

void ParticleEditor::DebugAll() {
    if (emitters_.empty()) {
        ImGui::Text("エミッターがありません");
        return;
    }

    // エミッター名のリストを作成
    std::vector<std::string> emitterNames;
    for (const auto &[name, emitter] : emitters_) {
        emitterNames.push_back(name);
    }

    // インデックスの範囲チェック
    if (selectedEmitterIndex_ >= emitterNames.size()) {
        selectedEmitterIndex_ = std::max(0, (int)emitterNames.size() - 1);
    }

    // エミッター選択用のCombo
    std::vector<const char *> emitterNameCStrs;
    for (const auto &name : emitterNames) {
        emitterNameCStrs.push_back(name.c_str());
    }

    if (ImGui::Combo("エミッター選択", &selectedEmitterIndex_,
                     emitterNameCStrs.data(), (int)emitterNameCStrs.size())) {
        // 選択が変更された場合、選択されたエミッター名を更新
        selectedEmitterName_ = emitterNames[selectedEmitterIndex_];
    }

    // 初回選択時の処理
    if (selectedEmitterName_.empty() && !emitterNames.empty()) {
        selectedEmitterName_ = emitterNames[selectedEmitterIndex_];
    }

    // 選択されたエミッターのDebugを実行
    if (!selectedEmitterName_.empty()) {
        auto it = emitters_.find(selectedEmitterName_);
        if (it != emitters_.end() && it->second) {
            it->second->Debug();
        }
    }
}

//std::unique_ptr<ParticleEmitter> ParticleEditor::GetEmitter(const std::string &name) {
//    auto it = emitters_.find(name);
//    if (it != emitters_.end()) {
//        // マップから取り出し、所有権を呼び出し元に移動
//        return std::move(it->second);
//    }
//    return nullptr;
//}

std::unique_ptr<ParticleEmitter> ParticleEditor::CreateEmitterFromTemplate(const std::string &name) {
    auto it = emitters_.find(name);
    if (it != emitters_.end() && it->second) {
        return it->second->Clone(); // コピーを作って返す
    }
    return nullptr;
}

void ParticleEditor::EditorWindow() {
    ImGui::Begin("パーティクルエディター");
    ShowImGuiEditor();
    ImGui::End();
}

// カラー付きCollapsingHeaderを表示するヘルパー関数
bool ParticleEditor::ColoredCollapsingHeader(const char *label, int colorIndex) {
    // 現在のImGuiカラーを保存
    ImVec4 originalColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);

    // 色を設定
    ImGui::PushStyleColor(ImGuiCol_Header, headerColors_[colorIndex % 6]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(
                                                      headerColors_[colorIndex % 6].x + 0.1f,
                                                      headerColors_[colorIndex % 6].y + 0.1f,
                                                      headerColors_[colorIndex % 6].z + 0.1f,
                                                      0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(
                                                     headerColors_[colorIndex % 6].x + 0.2f,
                                                     headerColors_[colorIndex % 6].y + 0.2f,
                                                     headerColors_[colorIndex % 6].z + 0.2f,
                                                     1.0f));

    // CollapsingHeaderを表示
    bool opened = ImGui::CollapsingHeader(label);

    // 設定した色をリセット
    ImGui::PopStyleColor(3);

    return opened;
}

void ParticleEditor::ShowImGuiEditor() {
    if (ImGui::BeginTabBar("パーティクル")) {
        if (ImGui::BeginTabItem("パーティクル作成")) {

            // エミッター追加のCollapsingHeader
            if (ColoredCollapsingHeader("エミッター追加", 0)) {
                // 名前の入力
                char nameBuffer[256];
                strcpy_s(nameBuffer, sizeof(nameBuffer), localEmitterName_.c_str());
                ImGui::Text("エミッターの名前");
                if (ImGui::InputText(" ", nameBuffer, sizeof(nameBuffer))) {
                    localEmitterName_ = std::string(nameBuffer);
                }

                // エミッター作成ボタン
                ImGui::Spacing();
                if (!localEmitterName_.empty()) {
                    if (ImGui::Button("エミッター生成")) {
                        AddParticleEmitter(localEmitterName_);
                        localEmitterName_.clear();
                    }
                }
            }

            // パーティクルグループ作成のCollapsingHeader
            if (ColoredCollapsingHeader("パーティクルグループ作成", 1)) {
                // 名前の入力
                char nameBuffer[256];
                strcpy_s(nameBuffer, sizeof(nameBuffer), localName_.c_str());
                ImGui::Text("パーティクルグループの名前");
                if (ImGui::InputText("  ", nameBuffer, sizeof(nameBuffer))) {
                    localName_ = std::string(nameBuffer);
                }

                // パーティクルタイプ選択（ラジオボタン）
                ImGui::Spacing();
                ImGui::Text("パーティクルタイプ選択");

                static int selectedType = 0; // 0: モデル, 1: プリミティブ
                ImGui::RadioButton("モデルパーティクル", &selectedType, 0);
                ImGui::SameLine();
                ImGui::RadioButton("プリミティブモデル", &selectedType, 1);
                ImGui::Separator();

                // モデルパーティクル選択時
                if (selectedType == 0) {
                    // モデル選択セクション (青色)
                    if (ColoredCollapsingHeader("モデル選択", 2)) {
                        // モデルファイル選択
                        static std::filesystem::path baseDirObj = "resources/models/";
                        static std::filesystem::path currentDirObj = "resources/models";
                        static std::string selectedFolderObj = "";
                        static std::string selectedFileObj = "";

                        // 「戻る」ボタン（上の階層に戻る）
                        if (currentDirObj != "resources/models") {
                            if (ImGui::Button("< 戻る(Model)")) {
                                currentDirObj = currentDirObj.parent_path();
                                selectedFolderObj = "";
                                selectedFileObj = "";
                            }
                        }

                        // フォルダ一覧
                        std::vector<std::string> foldersObj;
                        std::vector<std::string> objFiles;

                        for (const auto &entry : std::filesystem::directory_iterator(currentDirObj)) {
                            if (entry.is_directory()) {
                                foldersObj.push_back(entry.path().filename().string());
                            } else if (entry.path().extension() == ".obj") {
                                objFiles.push_back(entry.path().filename().string());
                            }
                        }

                        // フォルダ選択 (クリックで移動)
                        if (!foldersObj.empty()) {
                            ImGui::Text("フォルダ");
                            ImGui::Separator();
                            for (const auto &folder : foldersObj) {
                                std::string folderNameTex = folder + " (Model)"; // フォルダ名に "(Model)" を追加
                                if (ImGui::Selectable(folderNameTex.c_str(), selectedFolderObj == folder)) {
                                    selectedFolderObj = folderNameTex;
                                    currentDirObj = currentDirObj / folder; // フォルダ移動
                                    selectedFileObj = "";                   // 新しいフォルダを開いたらファイル選択をリセット
                                }
                                ImGui::Separator();
                            }
                        }

                        // `.obj` ファイル選択
                        if (!objFiles.empty()) {
                            ImGui::Text("モデルファイル:");
                            if (ImGui::BeginCombo("ファイル選択", selectedFileObj.empty() ? "なし" : selectedFileObj.c_str())) {
                                for (const auto &file : objFiles) {
                                    bool isSelected = (file == selectedFileObj);
                                    if (ImGui::Selectable(file.c_str(), isSelected)) {
                                        selectedFileObj = file;

                                        // `baseDirObj` からの相対パスを取得
                                        std::filesystem::path relativePath = (currentDirObj / file).lexically_relative(baseDirObj);

                                        // Windowsのバックスラッシュをスラッシュに変換
                                        std::string pathStr = relativePath.string();
                                        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                                        // `fileNameObj_` に保存
                                        localFileObj_ = pathStr;
                                    }
                                    if (isSelected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }

                    // テクスチャ選択セクション (緑色)
                    if (ColoredCollapsingHeader("テクスチャ選択", 3)) {
#ifdef _DEBUG
                        ShowTextureFile(localTexturePath_);
#endif // _DEBUG
                    }

                    // パーティクルグループ作成ボタン
                    ImGui::Spacing();
                    if (!localName_.empty() && !localFileObj_.empty()) {
                        if (ImGui::Button("モデルパーティクルグループ生成")) {
                            AddParticleGroup(localName_, localFileObj_, localTexturePath_);
                            localName_.clear();
                            localFileObj_.clear();
                            localTexturePath_.clear(); // テクスチャのパスもクリア
                        }
                    }
                }
                // プリミティブモデル選択時
                else if (selectedType == 1) {
                    // プリミティブタイプ選択セクション (紫色)
                    if (ColoredCollapsingHeader("プリミティブタイプ選択", 4)) {
                        const char *primitiveType[] = {"未選択", "プレーン", "球", "キューブ", "シリンダー", "リング", "三角形", "円錐", "四角錐"};
                        int currentPrimitiveType = static_cast<int>(localType_);
                        // 初期値が未選択（None = -1）の場合に対応するため +1 して選択肢に表示
                        if (ImGui::Combo("タイプ選択", &currentPrimitiveType, primitiveType, IM_ARRAYSIZE(primitiveType))) {
                            localType_ = static_cast<PrimitiveType>(currentPrimitiveType);
                        }
                    }

                    // テクスチャ選択セクション (オレンジ色)
                    if (ColoredCollapsingHeader("テクスチャ選択", 5)) {
#ifdef _DEBUG
                        ShowTextureFile(localTexturePath_);
#endif // _DEBUG
                    }

                    // パーティクルグループ作成ボタン
                    ImGui::Spacing();
                    if (!localName_.empty()) {
                        // localType_ が None（未選択）のときはボタンを無効化
                        bool isTypeInvalid = (localType_ == PrimitiveType::None);
                        if (isTypeInvalid) {
                            ImGui::BeginDisabled();
                        }

                        if (ImGui::Button("プリミティブパーティクルグループ生成")) {
                            AddPrimitiveParticleGroup(localName_, localTexturePath_, localType_);
                            localName_.clear();
                            localTexturePath_.clear();        // テクスチャのパスもクリア
                            localType_ = PrimitiveType::None; // 初期化
                        }

                        if (isTypeInvalid) {
                            ImGui::EndDisabled();
                        }
                    }
                }
            }

            // パーティクルデータのロードセクション (黄色系)
            if (ColoredCollapsingHeader("パーティクルデータのロード", 2)) {
                ShowFileSelector();
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void ParticleEditor::ShowFileSelector() {
    static int selectedIndex = -1;
    std::vector<std::string> jsonFiles = GetJsonFiles();

    // JSONファイルがない場合のチェック
    if (jsonFiles.empty()) {
        ImGui::Text("Jsonファイルが見つかりませんでした");
        return;
    }

    // ファイルリストをCスタイル文字列の配列に変換
    std::vector<const char *> fileNames;
    for (const auto &filePath : jsonFiles) {
        fileNames.push_back(filePath.c_str());
    }

    ImGui::Text("Jsonファイルの選択:");
    ImGui::Separator();

    // Comboボックスでファイル選択
    if (ImGui::Combo("JSON Files", &selectedIndex, fileNames.data(), static_cast<int>(fileNames.size()))) {
        // ファイル選択時の動作（選択されたファイル名を表示）
        if (selectedIndex >= 0) {
            ImGui::Text("ファイル選択:");
            ImGui::TextWrapped("%s", jsonFiles[selectedIndex].c_str());
        }
    }

    // ボタンでパーティクルデータをセット
    if (selectedIndex >= 0 && ImGui::Button("パーティクルデータのセット")) {
        isLoad_ = true;
        // name_ に ".json" を除いた名前を設定
        std::string selectedFileName = jsonFiles[selectedIndex];
        name_ = selectedFileName.substr(0, selectedFileName.find_last_of('.')); // ".json" を除去
        AddParticleEmitter(name_, fileName_, texturePath_);
        isLoad_ = false;
    }
}

std::vector<std::string> ParticleEditor::GetJsonFiles() {
    static std::vector<std::string> jsonFiles; // キャッシュされたJSONファイルリスト
    static size_t lastFileCount = 0;           // 最後に取得したJSONファイル数
    std::filesystem::path baseDir = "resources/jsons/Particle";

    // ディレクトリが存在しない場合はキャッシュをクリア
    if (!std::filesystem::exists(baseDir) || !std::filesystem::is_directory(baseDir)) {
        jsonFiles.clear();
        lastFileCount = 0;
        return jsonFiles;
    }

    // 現在のファイル数をカウント
    size_t currentFileCount = std::count_if(
        std::filesystem::directory_iterator(baseDir),
        std::filesystem::directory_iterator{},
        [](const std::filesystem::directory_entry &entry) {
            return entry.path().extension() == ".json";
        });

    // ファイル数が変わった場合のみ更新
    if (currentFileCount != lastFileCount) {
        jsonFiles.clear(); // リストをクリア
        for (const auto &entry : std::filesystem::directory_iterator(baseDir)) {
            if (entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path().filename().string());
            }
        }
        lastFileCount = currentFileCount; // 更新したファイル数を記録
    }

    return jsonFiles;
}

