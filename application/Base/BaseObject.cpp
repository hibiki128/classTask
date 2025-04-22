#include "BaseObject.h"

void BaseObject::Init(const std::string className) {
    objectName_ = className;
    /// ワールドトランスフォームの初期化
    transform_.Initialize();
    // カラーのセット
    objColor_.Initialize();
    objColor_.SetColor(Vector4(1, 1, 1, 1));
    // ライティングのセット
    isLighting_ = true;
    isCollider = false;

    LoadFromJson();
    AnimaLoadFromJson();
}

void BaseObject::Update() {

    // 元となるワールドトランスフォームの更新
    transform_.UpdateMatrix();
    /// 色転送
    objColor_.TransferMatrix();
    if (obj3d_->GetHaveAnimation()) {
        obj3d_->AnimationUpdate(isLoop_);
    }
}

void BaseObject::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    // オフセットを加える前の現在の位置を取得
    Vector3 currentPosition = transform_.translation_;

    // オフセットを加えて新しい位置を計算
    Vector3 newPosition = currentPosition + offSet;

    // 新しい位置を設定
    transform_.translation_ = newPosition;

    // オブジェクトの描画
    obj3d_->Draw(transform_, viewProjection, &objColor_, isLighting_);

    // スケルトンの描画が必要な場合
    if (skeletonDraw_) {
        obj3d_->DrawSkeleton(transform_, viewProjection);
    }

    // 描画後に元の位置に戻す場合は、以下の行を追加
    transform_.translation_ = currentPosition;
}

Vector3 BaseObject::GetWorldPosition() const {
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得
    worldPos.x = transform_.matWorld_.m[3][0];
    worldPos.y = transform_.matWorld_.m[3][1];
    worldPos.z = transform_.matWorld_.m[3][2];

    return worldPos;
}

void BaseObject::CreateModel(const std::string modelname) {
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->CreateModel(modelname);
}

void BaseObject::CreatePrimitiveModel(const PrimitiveType &type) {
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->CreatePrimitiveModel(type);
}

void BaseObject::CreateCollider() {
    Collider::Initialize(objectName_);
    isCollider = true;
}

void BaseObject::DebugImGui() {

    if (ImGui::BeginTabBar(objectName_.c_str())) {
        DebugTransform();
        if (isCollider) {
            DebugCollider();
        }
        ImGui::EndTabBar();
    }
}

void BaseObject::DebugTransform() {
    if (ImGui::BeginTabItem((objectName_ + "トランスフォーム").c_str())) {
        ImGui::DragFloat3("位置", &transform_.translation_.x, 0.1f);
        float rotationDegrees[3] = {
            radiansToDegrees(transform_.rotation_.x),
            radiansToDegrees(transform_.rotation_.y),
            radiansToDegrees(transform_.rotation_.z)};
        if (ImGui::DragFloat3("回転", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
            // 操作後、度数法からラジアンに戻して保存
            transform_.rotation_.x = degreesToRadians(rotationDegrees[0]);
            transform_.rotation_.y = degreesToRadians(rotationDegrees[1]);
            transform_.rotation_.z = degreesToRadians(rotationDegrees[2]);
        }
        ImGui::DragFloat3("大きさ", &transform_.scale_.x, 0.1f);
        if (ImGui::Button("セーブ")) {
            SaveToJson();
            std::string message = std::format("Transform saved.");
            MessageBoxA(nullptr, message.c_str(), "Object", 0);
        }
        ImGui::EndTabItem();
    }
    if (obj3d_->GetHaveAnimation()) {
        if (ImGui::BeginTabItem((objectName_ + "アニメーション").c_str())) {
            ImGui::Checkbox("ループ", &isLoop_);
            ImGui::Checkbox("スケルトン描画", &skeletonDraw_);
            if (ImGui::Button("アニメーション再生")) {
                obj3d_->PlayAnimation();
            }
            if (ImGui::TreeNode("アニメーションセット")) {
                ShowFileSelector();
                ImGui::TreePop();
            }
            if (ImGui::Button("セーブ")) {
                AnimaSaveToJson();
                std::string message = std::format("Anima saved.");
                MessageBoxA(nullptr, message.c_str(), "Object", 0);
            }
            ImGui::EndTabItem();
        }
    }
}

void BaseObject::DebugCollider() {
    Collider::OffsetImgui();
}

Vector3 BaseObject::GetCenterPosition() const {
    return transform_.translation_;
}

Vector3 BaseObject::GetCenterRotation() const {
    return transform_.rotation_;
}

void BaseObject::SaveToJson() {
    TransformDatas_->Save<Vector3>("translation", transform_.translation_);
    TransformDatas_->Save<Vector3>("rotation", transform_.rotation_);
    TransformDatas_->Save<Vector3>("scale", transform_.scale_);
}

void BaseObject::LoadFromJson() {
    TransformDatas_ = std::make_unique<DataHandler>("Transform", objectName_);
    transform_.translation_ = TransformDatas_->Load<Vector3>("translation", {0.0f, 0.0f, 0.0f});
    transform_.rotation_ = TransformDatas_->Load<Vector3>("rotation", {0.0f, 0.0f, 0.0f});
    transform_.scale_ = TransformDatas_->Load<Vector3>("scale", {1.0f, 1.0f, 1.0f});
}

void BaseObject::AnimaSaveToJson() {
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
