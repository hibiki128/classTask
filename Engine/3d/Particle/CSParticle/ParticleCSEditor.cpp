#define NOMINMAX
#include "ParticleCSEditor.h"
#include "Debug/ImGui/ImGuiManager.h"
#ifdef _DEBUG
#include "ShowFolder/ShowFolder.h"
#endif // _DEBUG

ParticleCSEditor *ParticleCSEditor::instance_ = nullptr;

ParticleCSEditor *ParticleCSEditor::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new ParticleCSEditor();
    }
    return instance_;
}

void ParticleCSEditor::Finalize() {
    delete instance_;
    instance_ = nullptr;
}

void ParticleCSEditor::Initialize() {
    groupManager_ = ParticleCSGroupManager::GetInstance();
    SetupColors();
}

void ParticleCSEditor::SetupColors() {
    headerColors_[0] = ImVec4(0.2f, 0.4f, 0.8f, 0.8f); // 青系
    headerColors_[1] = ImVec4(0.8f, 0.4f, 0.2f, 0.8f); // オレンジ系
    headerColors_[2] = ImVec4(0.2f, 0.7f, 0.4f, 0.8f); // 緑系
    headerColors_[3] = ImVec4(0.7f, 0.3f, 0.7f, 0.8f); // 紫系
    headerColors_[4] = ImVec4(0.7f, 0.7f, 0.2f, 0.8f); // 黄色系
    headerColors_[5] = ImVec4(0.5f, 0.5f, 0.5f, 0.8f); // グレー系
}

void ParticleCSEditor::AddEmitter(const std::string &name) {
    auto emitter = std::make_unique<ParticleCSEmitter>();
    emitter->Initialize(name);
    emitters_[name] = std::move(emitter);
}

void ParticleCSEditor::AddEmitter(const std::string &name, const std::string &fileName, const std::string &texturePath) {
    auto emitter = std::make_unique<ParticleCSEmitter>();
    emitter->Initialize(name);
    emitters_[name] = std::move(emitter);
}

void ParticleCSEditor::CreateGroup(const std::string &groupName, const std::string &modelPath, const std::string &texturePath) {
    groupManager_->CreateModelGroup(groupName, modelPath, texturePath);
}

void ParticleCSEditor::CreatePrimitiveGroup(const std::string &groupName, PrimitiveType primitiveType, const std::string &texturePath) {
    groupManager_->CreatePrimitiveGroup(groupName, primitiveType, texturePath);
}

void ParticleCSEditor::SetExternalParticleCount(const std::string &baseName, size_t count) {
    if (currentFrameNumber_ != lastUpdateFrame_) {
        currentFrameStats_.clear();
        lastUpdateFrame_ = currentFrameNumber_;
    }

    currentFrameStats_[baseName].count += count;
    currentFrameStats_[baseName].instanceCount++;
}

void ParticleCSEditor::UpdateFrameStats() {
    displayStats_ = currentFrameStats_;
    currentFrameNumber_++;
}

void ParticleCSEditor::DrawAll(const ViewProjection &vp) {
    for (auto &[name, emitter] : emitters_) {
        if (emitter) {
            if (emitter->IsAuto()) {
                emitter->Update();
            }
            emitter->Draw(vp);
        }
    }
}

void ParticleCSEditor::DebugAll() {
    if (emitters_.empty()) {
        ImGui::Text("コンピュートシェーダーエミッターがありません");
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

void ParticleCSEditor::EditorWindow() {
    ImGui::Begin("コンピュートシェーダーパーティクルエディター");
    ShowImGuiEditor();
    ImGui::End();
}

void ParticleCSEditor::ShowImGuiEditor() {
    if (ImGui::BeginTabBar("CSパーティクル")) {
        if (ImGui::BeginTabItem("CSパーティクル作成")) {

            // エミッター追加のCollapsingHeader
            if (ColoredCollapsingHeader("CSエミッター追加", 0)) {
                char nameBuffer[256];
                strcpy_s(nameBuffer, sizeof(nameBuffer), localEmitterName_.c_str());
                ImGui::Text("エミッターの名前");
                if (ImGui::InputText("##emitter_name", nameBuffer, sizeof(nameBuffer))) {
                    localEmitterName_ = std::string(nameBuffer);
                }

                ImGui::Spacing();
                if (!localEmitterName_.empty()) {
                    if (ImGui::Button("CSエミッター生成")) {
                        AddEmitter(localEmitterName_);
                        localEmitterName_.clear();
                    }
                }
            }

            // パーティクルグループ作成のCollapsingHeader
            if (ColoredCollapsingHeader("CSパーティクルグループ作成", 1)) {
                char nameBuffer[256];
                strcpy_s(nameBuffer, sizeof(nameBuffer), localGroupName_.c_str());
                ImGui::Text("パーティクルグループの名前");
                if (ImGui::InputText("##group_name", nameBuffer, sizeof(nameBuffer))) {
                    localGroupName_ = std::string(nameBuffer);
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
                    if (ColoredCollapsingHeader("モデル選択", 2)) {
                        ShowModelSelector();
                    }

                    if (ColoredCollapsingHeader("テクスチャ選択", 3)) {
#ifdef _DEBUG
                        ShowTextureFile(localTexturePath_);
#endif
                    }

                    ImGui::Spacing();
                    if (!localGroupName_.empty() && !localModelPath_.empty()) {
                        if (ImGui::Button("モデルCSパーティクルグループ生成")) {
                            CreateGroup(localGroupName_, localModelPath_, localTexturePath_);
                            localGroupName_.clear();
                            localModelPath_.clear();
                            localTexturePath_.clear();
                        }
                    }
                }
                // プリミティブモデル選択時
                else if (selectedType == 1) {
                    if (ColoredCollapsingHeader("プリミティブタイプ選択", 4)) {
                        const char *primitiveTypes[] = {"未選択", "プレーン", "球", "キューブ", "シリンダー", "リング", "三角形", "円錐", "四角錐"};
                        int currentPrimitiveType = static_cast<int>(localPrimitiveType_);
                        if (ImGui::Combo("タイプ選択", &currentPrimitiveType, primitiveTypes, IM_ARRAYSIZE(primitiveTypes))) {
                            localPrimitiveType_ = static_cast<PrimitiveType>(currentPrimitiveType);
                        }
                    }

                    if (ColoredCollapsingHeader("テクスチャ選択", 5)) {
#ifdef _DEBUG
                        ShowTextureFile(localTexturePath_);
#endif
                    }

                    ImGui::Spacing();
                    if (!localGroupName_.empty()) {
                        bool isTypeInvalid = (localPrimitiveType_ == PrimitiveType::None);
                        if (isTypeInvalid) {
                            ImGui::BeginDisabled();
                        }

                        if (ImGui::Button("プリミティブCSパーティクルグループ生成")) {
                            CreatePrimitiveGroup(localGroupName_, localPrimitiveType_, localTexturePath_);
                            localGroupName_.clear();
                            localTexturePath_.clear();
                            localPrimitiveType_ = PrimitiveType::None;
                        }

                        if (isTypeInvalid) {
                            ImGui::EndDisabled();
                        }
                    }
                }
            }

            // グループ管理セクション
            if (ColoredCollapsingHeader("グループ管理", 2)) {
                ShowGroupManagement();
            }

            // 統計情報
            SceneParticleCount();

            // 全パーティクルを止めるボタン
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20.0f, 10.0f));

            if (ImGui::Button("全CSパーティクルを止める", ImVec2(200, 40))) {
                for (auto &emitter : emitters_) {
                    emitter.second->SetAuto(false);
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("CSエミッター設定")) {
            DebugAll();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void ParticleCSEditor::ShowModelSelector() {
    static std::filesystem::path baseDir = "resources/models/";
    static std::filesystem::path currentDir = "resources/models";
    static std::string selectedFolder = "";
    static std::string selectedFile = "";

    // 「戻る」ボタン（上の階層に戻る）
    if (currentDir != "resources/models") {
        if (ImGui::Button("< 戻る(Model)")) {
            currentDir = currentDir.parent_path();
            selectedFolder = "";
            selectedFile = "";
        }
    }

    // フォルダ一覧
    std::vector<std::string> folders;
    std::vector<std::string> objFiles;

    for (const auto &entry : std::filesystem::directory_iterator(currentDir)) {
        if (entry.is_directory()) {
            folders.push_back(entry.path().filename().string());
        } else if (entry.path().extension() == ".obj") {
            objFiles.push_back(entry.path().filename().string());
        }
    }

    // フォルダ選択（クリックで移動）
    if (!folders.empty()) {
        ImGui::Text("フォルダ");
        ImGui::Separator();
        for (const auto &folder : folders) {
            std::string folderName = folder + " (Model)";
            if (ImGui::Selectable(folderName.c_str(), selectedFolder == folder)) {
                selectedFolder = folderName;
                currentDir = currentDir / folder;
                selectedFile = "";
            }
            ImGui::Separator();
        }
    }

    // .objファイル選択
    if (!objFiles.empty()) {
        ImGui::Text("モデルファイル:");
        if (ImGui::BeginCombo("ファイル選択", selectedFile.empty() ? "なし" : selectedFile.c_str())) {
            for (const auto &file : objFiles) {
                bool isSelected = (file == selectedFile);
                if (ImGui::Selectable(file.c_str(), isSelected)) {
                    selectedFile = file;

                    // 相対パスを取得
                    std::filesystem::path relativePath = (currentDir / file).lexically_relative(baseDir);
                    std::string pathStr = relativePath.string();
                    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                    localModelPath_ = pathStr;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
}

void ParticleCSEditor::ShowGroupManagement() {
    auto groupNames = groupManager_->GetGroupNames();

    ImGui::Text("既存のグループ:");
    if (groupNames.empty()) {
        ImGui::Text("グループがありません");
        return;
    }

    for (const auto &groupName : groupNames) {
        auto group = groupManager_->GetGroup(groupName);
        if (group) {
            ImGui::Text("- %s", groupName.c_str());
            ImGui::SameLine();

            // グループ削除ボタン
            if (ImGui::SmallButton(("削除##" + groupName).c_str())) {
                groupManager_->RemoveGroup(groupName);
                break;
            }

            ImGui::SameLine();

            // エミッターにアタッチボタン
            if (ImGui::SmallButton(("アタッチ##" + groupName).c_str())) {
                if (!selectedEmitterName_.empty()) {
                    auto it = emitters_.find(selectedEmitterName_);
                    if (it != emitters_.end() && it->second) {
                        it->second->AttachGroup(groupName);
                    }
                }
            }

            // グループ情報の表示
            ImGui::Indent();
            ImGui::Text("  パーティクル数: %zu", group->GetActiveParticleCount());
            if (!group->GetModelPath().empty()) {
                ImGui::Text("  モデル: %s", group->GetModelPath().c_str());
            } else {
                ImGui::Text("  プリミティブ: %d", static_cast<int>(group->GetPrimitiveType()));
            }
            if (!group->GetTexturePath().empty()) {
                ImGui::Text("  テクスチャ: %s", group->GetTexturePath().c_str());
            }
            ImGui::Unindent();
        }
    }
}

void ParticleCSEditor::SceneParticleCount() {
    if (ImGui::CollapsingHeader("CSパーティクル統計")) {
        size_t grandTotal = 0;
        size_t totalInstances = 0;

        // 合計を計算
        for (const auto &[name, stats] : displayStats_) {
            grandTotal += stats.count;
            totalInstances += stats.instanceCount;
        }

        // ヘッダー情報
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "合計: %zu個", grandTotal);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "(%zu種類)", displayStats_.size());

        if (!displayStats_.empty()) {
            ImGui::Separator();

            // シンプルなリスト表示
            for (const auto &[name, stats] : displayStats_) {
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", name.c_str());
                ImGui::SameLine();
                ImGui::Text(": %zu", stats.count);

                if (stats.instanceCount > 1) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "×%zu", stats.instanceCount);
                }
            }
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "エミッターなし");
        }

        // グループマネージャーからの統計も表示
        size_t totalGroupParticles = groupManager_->GetTotalParticleCount();
        if (totalGroupParticles > 0) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.4f, 1.0f), "グループ総パーティクル数: %zu", totalGroupParticles);
        }
    }
}

bool ParticleCSEditor::ColoredCollapsingHeader(const char *label, int colorIndex) {
    ImVec4 originalColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);

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

    bool opened = ImGui::CollapsingHeader(label);

    ImGui::PopStyleColor(3);

    return opened;
}

std::unique_ptr<ParticleCSEmitter> ParticleCSEditor::CreateEmitterFromTemplate(const std::string &name) {
    auto it = emitters_.find(name);
    if (it != emitters_.end() && it->second) {
        return it->second->Clone();
    }
    return nullptr;
}

void ParticleCSEditor::LoadFromJson() {
    // JSONからのロード処理
    // 必要に応じて実装
}

std::vector<std::string> ParticleCSEditor::GetJsonFiles() {
    static std::vector<std::string> jsonFiles;
    static size_t lastFileCount = 0;
    std::filesystem::path baseDir = "resources/jsons/ParticleCS";

    if (!std::filesystem::exists(baseDir) || !std::filesystem::is_directory(baseDir)) {
        jsonFiles.clear();
        lastFileCount = 0;
        return jsonFiles;
    }

    size_t currentFileCount = std::count_if(
        std::filesystem::directory_iterator(baseDir),
        std::filesystem::directory_iterator{},
        [](const std::filesystem::directory_entry &entry) {
            return entry.path().extension() == ".json";
        });

    if (currentFileCount != lastFileCount) {
        jsonFiles.clear();
        for (const auto &entry : std::filesystem::directory_iterator(baseDir)) {
            if (entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path().filename().string());
            }
        }
        lastFileCount = currentFileCount;
    }

    return jsonFiles;
}