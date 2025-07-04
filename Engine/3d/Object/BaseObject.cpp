#define NOMINMAX
#include "BaseObject.h"
#include "Scene/SceneManager.h"
#include "ShowFolder/ShowFolder.h"

void BaseObject::Init(const std::string objectName) {
    transform_ = std::make_unique<WorldTransform>();
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->Initialize();
    objectName_ = objectName;
    /// ワールドトランスフォームの初期化
    transform_->Initialize();
    // カラーのセット
    objColor_.Initialize();
    objColor_.GetColor() = Vector4(1, 1, 1, 1);
    // ライティングのセット
    isLighting_ = true;
    isCollider = false;
    LoadFromJson();
    AnimaLoadFromJson();
}

void BaseObject::Update() {
    /// 色転送
    objColor_.TransferMatrix();
    if (obj3d_->GetHaveAnimation()) {
        obj3d_->AnimationUpdate(isLoop_);
    }
    SetBlendMode(blendMode_);
}

void BaseObject::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    // オフセットを加える前の現在の位置を取得
    Vector3 currentPosition = transform_->translation_;

    // オフセットを加えて新しい位置を計算
    Vector3 newPosition = currentPosition + offSet;

    // 新しい位置を設定
    transform_->translation_ = newPosition;

    // スケルトンの描画が必要な場合
    if (skeletonDraw_) {
        obj3d_->DrawSkeleton(*transform_, viewProjection);
    }
    if (!isWireframe_) {
        if (isModelDraw_) {
            // オブジェクトの描画
            obj3d_->Draw(*transform_, viewProjection, reflect_, &objColor_, isLighting_);
        }
    } else {
        obj3d_->DrawWireframe(*transform_, viewProjection, isRainbow_);
    }

    // 描画後に元の位置に戻す場合は、以下の行を追加
    transform_->translation_ = currentPosition;
}

void BaseObject::UpdateWorldTransformHierarchy() {
    // まず自分のトランスフォームを更新
    if (transform_) {
        transform_->UpdateMatrix();
    }
    // 子を再帰的に更新
    for (auto it = children_.begin(); it != children_.end();) {
        BaseObject *child = *it;
        child->UpdateWorldTransformHierarchy();
        if (child->parent_ != this) {
            it = children_.erase(it);
        } else {
            ++it;
        }
    }
}

void BaseObject::UpdateHierarchy() {
    // 自分自身の処理
    Update();

    // 子リストをイテレート
    for (auto it = children_.begin(); it != children_.end();) {
        auto child = *it;
        // 再帰的に UpdateHierarchy
        child->UpdateHierarchy();

        // 子が「DetachParent()」した場合、parent_ == nullptr になる
        if (child->GetParent() != this) {
            // リストから削除
            it = children_.erase(it);
        } else {
            ++it;
        }
    }
}

Vector3 BaseObject::GetWorldPosition() const {
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos.x = transform_->matWorld_.m[3][0];
    worldPos.y = transform_->matWorld_.m[3][1];
    worldPos.z = transform_->matWorld_.m[3][2];

    return worldPos;
}

void BaseObject::SetParent(BaseObject *parent) {
    if (parent_ == parent) {
        return; // 同じ親を持ってる場合何もしない
    }
    if (parent_) {
        DetachParent(); // もし現在の親がいるなら一旦デタッチ
    }

    assert(parent != nullptr && "SetParent to nullptr is not allowed.");

    parent_ = parent;
    // 親の子リストに追加
    parent_->children_.push_back(this);

    if (transform_) {
        transform_->parent_ = parent->GetWorldTransform();
    }
}

void BaseObject::AddChild(BaseObject *child) {
    assert(child != nullptr && "AddChild is nullptr");
    child->SetParent(this);
}

void BaseObject::DetachParent() {
    if (parent_) {
        parent_ = nullptr;
        if (transform_) {
            transform_->parent_ = nullptr;
        }
    }
}

void BaseObject::DetachChild(BaseObject *child) {
    if (!child) {
        return;
    }
    if (child->parent_ != this) {
        return;
    }
    child->parent_ = nullptr;
    if (child->transform_) {
        child->transform_->parent_ = nullptr;
    }
    children_.remove(child);
}

BaseObject *BaseObject::GetParent() {
    return parent_;
}

std::list<BaseObject *> *BaseObject::GetChildren() {
    return &children_;
}

BaseObject *BaseObject::GetChildByName(const std::string &name) {
    for (auto &child : children_) {
        if (child->objectName_ == name) {
            return child;
        }
    }
    return nullptr;
}

void BaseObject::CreateModel(const std::string modelname) {
    obj3d_->CreateModel(modelname);
    modelPath_ = modelname;
    texturePath_ = obj3d_->GetTextureFilePath(0);
    LoadFromJson();
    AnimaLoadFromJson();
}

void BaseObject::CreatePrimitiveModel(const PrimitiveType &type) {
    obj3d_->CreatePrimitiveModel(type);
    isPrimitive_ = true;
}

void BaseObject::AddCollider() {
    colliders_.push_back(&Collider::AddCollider(objectName_));
    isCollider = true;
}

void BaseObject::ImGui() {

    if (ImGui::BeginTabBar(objectName_.c_str())) {
        if (ImGui::BeginTabItem(objectName_.c_str())) {
            DebugObject();

            // コライダーの設定
            if (isCollider) {
                DebugCollider();
            }
            if (ImGui::Button("コライダー追加")) {
                AddCollider();
            }

            // モデル描画チェックボックス
            bool modelDrawChanged = ImGui::Checkbox("モデル描画", &isModelDraw_);
            if (modelDrawChanged && isModelDraw_) {
                // モデル描画がオンになったときはワイヤーフレームをオフにする
                isWireframe_ = false;
            }

            // ワイヤーフレームチェックボックス
            bool wireframeChanged = ImGui::Checkbox("ワイヤーフレーム", &isWireframe_);
            if (wireframeChanged && isWireframe_) {
                // ワイヤーフレームがオンになったときはモデル描画をオフにする
                isModelDraw_ = false;
            }
            if (isWireframe_) {
                ImGui::Checkbox("???", &isRainbow_);
            } else {
                isRainbow_ = false;
            }

            // セーブボタン
            if (ImGui::Button("セーブ")) {
                SaveToJson();
                AnimaSaveToJson();
                for (auto &collider : colliders_) {
                    collider->SaveToJson();
                }
                std::string message = std::format("ObjectData saved.");
                MessageBoxA(nullptr, message.c_str(), "Object", 0);
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void BaseObject::DebugObject() {
    // 全体のスタイル設定
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

    // === トランスフォーム設定 ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.8f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.7f, 0.9f, 0.4f));

    if (ImGui::CollapsingHeader("トランスフォーム", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10);

        // 位置設定
        ImGui::AlignTextToFramePadding();
        ImGui::Text("位置:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::DragFloat3("##Position", &transform_->translation_.x, 0.1f, -1000.0f, 1000.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("リセット##ResetPos")) {
            transform_->translation_ = {0.0f, 0.0f, 0.0f};
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("位置をリセット");

        // 回転設定
        ImGui::AlignTextToFramePadding();
        ImGui::Text("回転:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);

        float rotationDegrees[3] = {
            radiansToDegrees(transform_->rotation_.x),
            radiansToDegrees(transform_->rotation_.y),
            radiansToDegrees(transform_->rotation_.z)};

        if (ImGui::DragFloat3("##Rotation", rotationDegrees, 1.0f, -360.0f, 360.0f, "%.1f°")) {
            transform_->rotation_.x = degreesToRadians(rotationDegrees[0]);
            transform_->rotation_.y = degreesToRadians(rotationDegrees[1]);
            transform_->rotation_.z = degreesToRadians(rotationDegrees[2]);
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("リセット##ResetRot")) {
            transform_->rotation_ = {0.0f, 0.0f, 0.0f};
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("回転をリセット");

        // スケール設定
        ImGui::AlignTextToFramePadding();
        ImGui::Text("大きさ:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::DragFloat3("##Scale", &transform_->scale_.x, 0.01f, 0.01f, 10.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("リセット##ResetScale")) {
            transform_->scale_ = {1.0f, 1.0f, 1.0f};
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("大きさをリセット");

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    // === マテリアル設定 ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.4f, 0.2f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.5f, 0.3f, 0.4f));

    if (ImGui::CollapsingHeader("マテリアル設定")) {
        ImGui::Indent(10);

        // カラー設定
        ImGui::Text("カラー:");
        ImGui::SameLine(80);
        Vector4 color = objColor_.GetColor();
        float colorArray[4] = {color.x, color.y, color.z, color.w};
        ImGui::PushItemWidth(200);
        if (ImGui::ColorEdit4("##ObjectColor", colorArray, ImGuiColorEditFlags_AlphaBar)) {
            objColor_.GetColor() = Vector4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // ライティング設定
        ImGui::Text("ライティング:");
        ImGui::SameLine(120);

        if (isLighting_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f));
            ImGui::Text("有効");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("無効");
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        if (ImGui::Button(isLighting_ ? "無効化" : "有効化")) {
            isLighting_ = !isLighting_;
        }

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    // === モデル設定 ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.2f, 0.8f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.7f, 0.3f, 0.9f, 0.4f));

    if (ImGui::CollapsingHeader("モデル設定")) {
        ImGui::Indent(10);

        static int selectedMaterialIndex = 0;
        size_t materialCount = obj3d_->GetMaterialCount();

        if (obj3d_->GetHaveAnimation() && materialCount > 1) {
            --materialCount;
        }

        // マテリアル選択
        if (materialCount > 1) {
            ImGui::Text("マテリアル:");
            ImGui::SameLine(120);

            std::vector<std::string> comboItems;
            for (int i = 0; i < static_cast<int>(materialCount); ++i) {
                comboItems.push_back("Material " + std::to_string(i + 1));
            }

            std::vector<const char *> comboItemsCStr;
            for (const auto &item : comboItems) {
                comboItemsCStr.push_back(item.c_str());
            }

            ImGui::PushItemWidth(150);
            if (ImGui::Combo("##MaterialIndex", &selectedMaterialIndex, comboItemsCStr.data(), static_cast<int>(comboItemsCStr.size()))) {
                // 選択変更時の処理
            }
            ImGui::PopItemWidth();

            selectedMaterialIndex = std::clamp(selectedMaterialIndex, 0, static_cast<int>(materialCount) - 1);
        } else {
            selectedMaterialIndex = 0;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("マテリアル: Single Material");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // テクスチャ設定
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.7f, 0.3f, 0.3f));
        if (ImGui::TreeNode("テクスチャ設定")) {
            ShowTextureFile(texturePath_);
            ImGui::Spacing();

            if (ImGui::Button("適用", ImVec2(80, 0))) {
                SetTexture(texturePath_, selectedMaterialIndex);
            }

            ImGui::SameLine();
            if (ImGui::Button("クリア", ImVec2(80, 0))) {
                texturePath_.clear();
            }

            ImGui::TreePop();
        }
        ImGui::PopStyleColor();

        // ブレンドモード設定
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.7f, 0.3f, 0.7f, 0.3f));
        if (ImGui::TreeNode("ブレンドモード")) {
            ShowBlendModeCombo(blendMode_);
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    // === アニメーション設定 ===
    if (obj3d_->GetHaveAnimation()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.6f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.7f, 0.3f, 0.4f));

        if (ImGui::CollapsingHeader("アニメーション設定")) {
            ImGui::Indent(10);

            // 制御オプション
            ImGui::Text("ループ:");
            ImGui::SameLine(80);
            ImGui::Checkbox("##Loop", &isLoop_);

            ImGui::Text("スケルトン:");
            ImGui::SameLine(100); // 幅を80から100に拡張
            ImGui::Checkbox("##Skeleton", &skeletonDraw_);

            ImGui::Spacing();

            // 制御ボタン
            if (ImGui::Button("再生", ImVec2(80, 0))) {
                obj3d_->PlayAnimation();
            }

            ImGui::Spacing();

            // アニメーションセット選択
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.8f, 0.2f, 0.3f));
            if (ImGui::TreeNode("アニメーションセット")) {
                ShowFileSelector();
                ImGui::TreePop();
            }
            ImGui::PopStyleColor();

            ImGui::Unindent(10);
        }

        ImGui::PopStyleColor(2);
    }

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);
}

void BaseObject::SaveToJson() {
    // JSONデータを扱うハンドラを作成
    ObjectDatas_ = std::make_unique<DataHandler>(foldarPath_, objectName_);
    modelPath_ = obj3d_->GetModelFilePath();
    texturePath_ = obj3d_->GetTextureFilePath(0);
    ObjectDatas_->Save<std::string>("modelName", modelPath_);
    ObjectDatas_->Save<std::string>("textureName", texturePath_);
    ObjectDatas_->Save<std::string>("objectName", objectName_);
    ObjectDatas_->Save<Vector3>("translation", transform_->translation_);
    ObjectDatas_->Save<Vector3>("rotation", transform_->rotation_);
    ObjectDatas_->Save<Vector3>("scale", transform_->scale_);
    ObjectDatas_->Save<bool>("Lighting", isLighting_);

    // カラーとライティング設定も保存
    Vector4 color = objColor_.GetColor();
    ObjectDatas_->Save<Vector4>("objectColor", color);
    ObjectDatas_->Save<bool>("isLighting", isLighting_);

    ObjectDatas_->Save<int>("blendMode", static_cast<int>(blendMode_));
}

void BaseObject::LoadFromJson() {
    // JSONデータを扱うハンドラを作成
    ObjectDatas_ = std::make_unique<DataHandler>(foldarPath_, objectName_);

    // 基本トランスフォームを読み込み
    transform_->translation_ = ObjectDatas_->Load<Vector3>("translation", {0.0f, 0.0f, 0.0f});
    transform_->rotation_ = ObjectDatas_->Load<Vector3>("rotation", {0.0f, 0.0f, 0.0f});
    transform_->scale_ = ObjectDatas_->Load<Vector3>("scale", {1.0f, 1.0f, 1.0f});
    isLighting_ = ObjectDatas_->Load<bool>("Lighting", true);

    // モデルパスが未設定でプリミティブでなければデフォルトモデルを使用
    if (modelPath_.empty() && !isPrimitive_) {
        modelPath_ = "debug/suzannu.obj";
    }

    // モデルパスをJSONから読み込み（上書きされる可能性あり）
    modelPath_ = ObjectDatas_->Load<std::string>("modelName", modelPath_);

    // テクスチャパスが未設定ならデフォルトにする
    if (texturePath_.empty()) {
        texturePath_ = "debug/uvChecker.png";
    }

    texturePath_ = ObjectDatas_->Load<std::string>("textureName", texturePath_);

    // オブジェクトカラーとライティングの設定を読み込み
    objColor_.GetColor() = ObjectDatas_->Load<Vector4>("objectColor", {1.0f, 1.0f, 1.0f, 1.0f});
    isLighting_ = ObjectDatas_->Load<bool>("isLighting", true);

    // ブレンドモードの設定を読み込み
    blendMode_ = static_cast<BlendMode>(ObjectDatas_->Load<int>("blendMode", 0));
}

void BaseObject::AnimaSaveToJson() {
    if (!AnimaDatas_) {
        return;
    }
    AnimaDatas_->Save<bool>("Loop", isLoop_);
}

void BaseObject::AnimaLoadFromJson() {
    AnimaDatas_ = std::make_unique<DataHandler>("Animation", objectName_);
    isLoop_ = AnimaDatas_->Load<bool>("Loop", false);
}

void BaseObject::ShowFileSelector() {
    static int selectedIndex = -1;                              // 選択中のインデックス（-1は未選択）
    static std::vector<std::string> gltfFiles = GetGltfFiles(); // GLTFファイルのリスト

    // ファイルリストをCスタイル文字列の配列に変換
    std::vector<const char *> fileNames;
    for (const auto &filePath : gltfFiles) {
        fileNames.push_back(filePath.c_str());
    }

    ImGui::Text("GLTFファイル選択");
    ImGui::Separator();

    // Comboボックスでファイル選択
    if (ImGui::Combo("GLTF Files", &selectedIndex, fileNames.data(), static_cast<int>(fileNames.size()))) {
        // ファイル選択時の動作（選択されたファイル名を表示）
        if (selectedIndex >= 0) {
            ImGui::Text("Selected File:");
            ImGui::TextWrapped("%s", gltfFiles[selectedIndex].c_str());
        }
    }

    // ボタンでアニメーションをセット
    if (selectedIndex >= 0 && ImGui::Button("Set Animation")) {
        obj3d_->SetAnimation(gltfFiles[selectedIndex]); // 選択されたファイルをSetAnimationに渡す
    }
}

void BaseObject::ShowBlendModeCombo(BlendMode &currentMode) {
    // コンボボックスに表示する項目（日本語）
    static const char *blendModeItems[] = {
        "なし",      // kNone
        "通常",      // kNormal
        "加算",      // kAdd
        "減算",      // kSubtract
        "乗算",      // kMultiply
        "スクリーン" // kScreen
    };

    // 現在の選択状態（enumをintにキャスト）
    int currentIndex = static_cast<int>(currentMode);

    // コンボボックス表示
    if (ImGui::Combo("ブレンドモード", &currentIndex, blendModeItems, IM_ARRAYSIZE(blendModeItems))) {
        // ユーザーが選択を変更したときに反映
        currentMode = static_cast<BlendMode>(currentIndex);
    }
}

std::vector<std::string> BaseObject::GetGltfFiles() {
    std::vector<std::string> gltfFiles;
    std::filesystem::path baseDir = "resources/models/animation"; // ベースディレクトリ
    for (const auto &entry : std::filesystem::directory_iterator(baseDir)) {
        if (entry.path().extension() == ".gltf") {
            // フルパスではなく相対パスを取得し、区切り文字をスラッシュに変更
            std::string relativePath = std::filesystem::relative(entry.path(), baseDir.parent_path()).string();
            std::replace(relativePath.begin(), relativePath.end(), '\\', '/'); // バックスラッシュをスラッシュに置換
            gltfFiles.push_back(relativePath);
        }
    }
    return gltfFiles;
}

void BaseObject::DebugCollider() {
    for (auto &collider : colliders_) {
        collider->OffsetImgui();
    }
}

Vector3 BaseObject::GetCenterPosition() const {
    return transform_->translation_;
}

Vector3 BaseObject::GetCenterRotation() const {
    return transform_->rotation_;
}
