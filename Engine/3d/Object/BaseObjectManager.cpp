#include "BaseObjectManager.h"
#ifdef _DEBUG
#include "Debug/ImGui/ImGuizmoManager.h"
#endif // _DEBUG
#include <ShowFolder/ShowFolder.h>

BaseObjectManager *BaseObjectManager::instance = nullptr;

BaseObjectManager *BaseObjectManager::GetInstance() {
    if (instance == nullptr) {
        instance = new BaseObjectManager();
    }
    return instance;
}

void BaseObjectManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void BaseObjectManager::RemoveAllObjects() {
    baseObjects_.clear();
#ifdef _DEBUG
    ImGuizmoManager::GetInstance()->DeleteTarget();
#endif // _DEBUG
}

void BaseObjectManager::AddObject(std::unique_ptr<BaseObject> baseObject) {
    const std::string &name = baseObject->GetName();
#ifdef _DEBUG
    ImGuizmoManager::GetInstance()->AddTarget(baseObject->GetName(), baseObject.get());
#endif // _DEBUG
    baseObjects_.emplace(name, std::move(baseObject));
}

void BaseObjectManager::Update() {
    for (auto &[name, obj] : baseObjects_) {
        obj->UpdateHierarchy();
        obj->UpdateWorldTransformHierarchy();
    }
}

void BaseObjectManager::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    for (auto &[name, obj] : baseObjects_) {
        obj->Draw(viewProjection, offSet);
    }
}

void BaseObjectManager::DrawImGui() {
    ImGui::Begin("オブジェクトエディター");

    // シーン保存モーダルを開くボタン
    if (ImGui::Button("シーン保存")) {
        ImGui::OpenPopup("シーン保存");
    }

    // モーダルウィンドウ（中央に表示、背景は自動で薄暗くなる）
    if (ImGui::BeginPopupModal("シーン保存", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("シーンの名前を入力してください");

        static char sceneNameBuffer[128] = "";

        // テキスト入力欄（sceneName_ を編集）
        ImGui::InputText("シーン名", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));

        // 横並びに「保存」ボタンと「キャンセル」ボタン
        if (ImGui::Button("保存", ImVec2(120, 0))) {
            sceneName_ = sceneNameBuffer; // 入力内容を保存
            SaveAll();                    // 実際の保存処理
            ImGui::CloseCurrentPopup();   // モーダルを閉じる
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup(); // キャンセル時も閉じる
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("シーン読み込み")) {
        ImGui::OpenPopup("シーン読み込み");
    }

    if (ImGui::BeginPopupModal("シーン読み込み", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("シーンの名前を入力してください");

        static char sceneNameBuffer[128] = "";

        // テキスト入力欄（sceneName_ を編集）
        ImGui::InputText("シーン名", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));

        // 横並びに「保存」ボタンと「キャンセル」ボタン
        if (ImGui::Button("読み込み", ImVec2(120, 0))) {
            sceneName_ = sceneNameBuffer; // 入力内容を保存
            LoadAll();                    // 実際の保存処理
            ImGui::CloseCurrentPopup();   // モーダルを閉じる
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup(); // キャンセル時も閉じる
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // オブジェクト生成モーダルを開くボタン
    if (ImGui::Button("オブジェクト生成")) {
        ImGui::OpenPopup("オブジェクト生成");
    }

    // オブジェクト生成モーダルウィンドウ
    if (ImGui::BeginPopupModal("オブジェクト生成", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("新しいオブジェクトを作成します");

        static char objectNameBuffer[128] = "";

        // オブジェクト名入力欄
        ImGui::InputText("オブジェクト名", objectNameBuffer, IM_ARRAYSIZE(objectNameBuffer));

        ImGui::Separator();

        // モデルファイル選択セクション
        ImGui::Text("モデルファイル選択:");
        ImGui::BeginChild("ModelFileSelector", ImVec2(600, 300), true);
        ShowModelFile(modelPath_);
        ImGui::EndChild();

        ImGui::Separator();

        // テクスチャファイル選択セクション
        ImGui::Text("テクスチャファイル選択 (オプション):");
        ImGui::BeginChild("TextureFileSelector", ImVec2(600, 300), true);
        ShowTextureFile(texturePath_);
        ImGui::EndChild();

        ImGui::Separator();

        // 選択状況の表示
        ImGui::Text("選択されたモデル: %s", modelPath_.empty() ? "未選択" : modelPath_.c_str());
        ImGui::Text("選択されたテクスチャ: %s", texturePath_.empty() ? "未選択" : texturePath_.c_str());

        ImGui::Separator();

        // 生成ボタンとキャンセルボタン
        bool canCreate = strlen(objectNameBuffer) > 0 && !modelPath_.empty();

        if (!canCreate) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        if (ImGui::Button("生成", ImVec2(120, 0))) {
            if (canCreate) {
                objectName_ = objectNameBuffer;
                CreateObject(objectName_, modelPath_, texturePath_);

                // 入力欄とパスをリセット
                memset(objectNameBuffer, 0, sizeof(objectNameBuffer));
                modelPath_ = "";
                texturePath_ = "";

                ImGui::CloseCurrentPopup();
            }
        }

        if (!canCreate) {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            // 入力欄とパスをリセット
            memset(objectNameBuffer, 0, sizeof(objectNameBuffer));
            modelPath_ = "";
            texturePath_ = "";

            ImGui::CloseCurrentPopup();
        }

        // 生成できない場合の理由を表示
        if (!canCreate) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "生成するには:");
            if (strlen(objectNameBuffer) == 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・オブジェクト名を入力してください");
            }
            if (modelPath_.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・モデルファイルを選択してください");
            }
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void BaseObjectManager::SaveAll() {
    for (auto &[name, obj] : baseObjects_) {
        obj->SetFolderPath("SceneData/" + sceneName_ + "/ObjectDatas");
        obj->SaveToJson();
    }
}

void BaseObjectManager::LoadAll() {
    // シーンデータのフォルダパスを構築
    std::string sceneDataPath = "Resources/jsons/SceneData/" + sceneName_ + "/ObjectDatas";

    // フォルダが存在するかチェック
    if (!std::filesystem::exists(sceneDataPath)) {
        // フォルダが存在しない場合は何もしない
        return;
    }

    // JSONファイルを検索
    std::vector<std::string> jsonFiles;
    for (const auto &entry : std::filesystem::directory_iterator(sceneDataPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path().filename().string());
        }
    }

    // 既存のオブジェクトをクリア
    RemoveAllObjects();

    // 各JSONファイルを読み込んでオブジェクトを生成
    for (const std::string &jsonFile : jsonFiles) {
        // JSONファイル名から拡張子を除去してオブジェクト名とする
        std::string objectName = jsonFile.substr(0, jsonFile.find_last_of('.'));

        // 新しいオブジェクトを作成
        std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();

        // フォルダパスを設定
        newObject->SetFolderPath("SceneData/" + sceneName_ + "/ObjectDatas");

        // オブジェクト名でInit

        newObject->Init(objectName);

        // 読み込んだデータからモデルとテクスチャのパスを取得
        std::string modelPath = newObject->GetModelPath();
        std::string texturePath = newObject->GetTexturePath();

        // モデルとテクスチャを設定
        if (!modelPath.empty()) {
            newObject->CreateModel(modelPath);
        }

        if (!texturePath.empty()) {
            newObject->SetTexture(texturePath, 0);
        }

        // オブジェクトマネージャーに追加
        this->AddObject(std::move(newObject));
    }
}

void BaseObjectManager::SetSceneName(std::string sceneName) {
    if (sceneName_ == sceneName) {
        return; // 既に同じシーン名が設定されている場合は何もしない
    }
    sceneName_ = sceneName;
}

void BaseObjectManager::CreateObject(std::string objectName, std::string modelPath, std::string texturePath) {
    std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();
    newObject->Init(objectName);
    newObject->CreateModel(modelPath);
    newObject->SetTexture(texturePath, 0);
    this->AddObject(std::move(newObject));
}

BaseObject *BaseObjectManager::GetObjectByName(const std::string &name) {
    auto it = baseObjects_.find(name);
    if (it != baseObjects_.end()) {
        return it->second.get();
    }
    return nullptr;
}
