#include "ParticleCSEmitter.h"
#include "Frame.h"
#include"ParticleCSEditor.h"
#include "line/DrawLine3D.h"
#include <algorithm>
#include <Data/DataHandler.h>

ParticleCSEmitter::ParticleCSEmitter() {}

ParticleCSEmitter::~ParticleCSEmitter() {}

void ParticleCSEmitter::Initialize(const std::string &name) {
    name_ = name;
    transform_.Initialize();
    groupManager_ = ParticleCSGroupManager::GetInstance();

    // データハンドラーの初期化
    if (!name.empty()) {
        datas_ = std::make_unique<DataHandler>("ParticleCS", name);
        LoadFromJson();
    }

    elapsedTime_ = 0.0f;
    emitFrequency_ = 1.0f / 60.0f; // デフォルト60FPS
    isActive_ = true;
    isVisible_ = true;
    isAuto_ = false;
}

void ParticleCSEmitter::Update() {
    if (!isActive_ || !isAuto_)
        return;

    // グループ更新を追加
    for (auto &groupName : attachedGroups_) {
        auto group = groupManager_->GetGroup(groupName);
        if (group) {
            group->Update();
        }
    }

    elapsedTime_ += Frame::DeltaTime();

    while (elapsedTime_ >= emitFrequency_) {
        Emit();
        elapsedTime_ -= emitFrequency_;
    }
}

void ParticleCSEmitter::UpdateOnce() {
    if (!isActive_) {
        Emit();
    }
}

void ParticleCSEmitter::Draw(const ViewProjection &vp) {
    transform_.UpdateMatrix();

    // 各グループに対して描画処理
    for (auto &groupName : attachedGroups_) {
        auto group = groupManager_->GetGroup(groupName);
        if (group) {
            // エミッター設定を更新
            UpdateEmitterSettings(group.get());
            group->Draw(vp);
        }
    }

    // エミッター形状の描画
    DrawEmitter();

    // 統計情報の更新
    UpdateStatistics();
}

void ParticleCSEmitter::Emit() {
    for (auto &groupName : attachedGroups_) {
        auto group = groupManager_->GetGroup(groupName);
        if (group) {
            // エミッター設定を更新してからエミット
            UpdateEmitterSettings(group.get());
            group->EmitParticles();
        }
    }
}

void ParticleCSEmitter::AttachGroup(const std::string &groupName) {
    auto it = std::find(attachedGroups_.begin(), attachedGroups_.end(), groupName);
    if (it == attachedGroups_.end()) {
        attachedGroups_.push_back(groupName);

        // グループが存在するか確認し、存在しない場合は作成
        if (!groupManager_->HasGroup(groupName)) {
            // デフォルトプリミティブグループを作成
            groupManager_->CreatePrimitiveGroup(groupName, PrimitiveType::Plane, "");
        }
    }
}

void ParticleCSEmitter::DetachGroup(const std::string &groupName) {
    auto it = std::find(attachedGroups_.begin(), attachedGroups_.end(), groupName);
    if (it != attachedGroups_.end()) {
        attachedGroups_.erase(it);
    }
}

void ParticleCSEmitter::UpdateEmitterSettings(ParticleCSGroup *group) {
    if (!group)
        return;

    // 基本的なエミッター設定を更新
    ParticleCSEmitterSettings settings;

    // トランスフォーム情報
    settings.position = transform_.translation_;
    settings.velocityMin = velocityMin_;
    settings.velocityMax = velocityMax_;
    settings.scaleMin = scaleMin_;
    settings.scaleMax = scaleMax_;
    settings.lifeTimeMin = lifeTimeMin_;
    settings.lifeTimeMax = lifeTimeMax_;

    // カラー設定
    settings.colorMode = colorMode_;
    settings.startColor = startColor_;
    settings.endColor = endColor_;

    // エミット設定
    settings.emitCount = emitCount_;
    settings.emitInterval = emitFrequency_;

    // グループのエミッター設定を更新
    group->UpdateEmitterSettings(0, settings);
}

void ParticleCSEmitter::DrawEmitter() {
    if (!isVisible_)
        return;

    // エミッター形状の描画（ワイヤーフレームボックス）
    std::array<Vector3, 8> localVertices = {
        Vector3{-0.5f, -0.5f, -0.5f}, Vector3{0.5f, -0.5f, -0.5f},
        Vector3{-0.5f, 0.5f, -0.5f}, Vector3{0.5f, 0.5f, -0.5f},
        Vector3{-0.5f, -0.5f, 0.5f}, Vector3{0.5f, -0.5f, 0.5f},
        Vector3{-0.5f, 0.5f, 0.5f}, Vector3{0.5f, 0.5f, 0.5f}};

    std::array<Vector3, 8> worldVertices;
    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale_, transform_.quateRotation_, transform_.translation_);

    for (size_t i = 0; i < localVertices.size(); ++i) {
        worldVertices[i] = Transformation(localVertices[i], worldMatrix);
    }

    // エッジを描画 - std::make_pairを明示的に使用
    constexpr std::array<std::pair<int, int>, 12> edges = {
        std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0),
        std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4),
        std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)};

    for (const auto &edge : edges) {
        DrawLine3D::GetInstance()->SetPoints(worldVertices[edge.first], worldVertices[edge.second]);
    }
}

void ParticleCSEmitter::UpdateStatistics() {
    size_t totalCount = 0;
    for (const auto &groupName : attachedGroups_) {
        auto group = groupManager_->GetGroup(groupName);
        if (group) {
            totalCount += group->GetActiveParticleCount();
        }
    }

    if (totalCount > 0) {
        ParticleCSEditor::GetInstance()->SetExternalParticleCount(name_, totalCount);
    }
}
void ParticleCSEmitter::Debug() {
    if (ImGui::CollapsingHeader("基本設定")) {
        ImGui::Checkbox("自動更新", &isAuto_);
        ImGui::Checkbox("アクティブ", &isActive_);
        ImGui::Checkbox("表示", &isVisible_);

        ImGui::SliderFloat("エミット頻度", &emitFrequency_, 0.01f, 1.0f);
        ImGui::SliderInt("エミット数", reinterpret_cast<int *>(&emitCount_), 1, 100);

        if (ImGui::Button("一回エミット")) {
            UpdateOnce();
        }
    }

    if (ImGui::CollapsingHeader("エミッターデータ")) {
        // トランスフォームデータ
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
        ImGui::Text("Transformデータ:");
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Columns(2, "TransformColumns", false);

        // 位置
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("位置");
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 0.5f));
        ImGui::DragFloat3("##位置", &transform_.translation_.x, 0.1f);
        ImGui::PopStyleColor();
        ImGui::NextColumn();

        // 回転
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("回転");
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        float rotationDegrees[3] = {
            radiansToDegrees(transform_.quateRotation_.x),
            radiansToDegrees(transform_.quateRotation_.y),
            radiansToDegrees(transform_.quateRotation_.z)};

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.3f, 0.4f, 0.5f));
        if (ImGui::DragFloat3("##回転 (度)", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
            transform_.quateRotation_.x = degreesToRadians(rotationDegrees[0]);
            transform_.quateRotation_.y = degreesToRadians(rotationDegrees[1]);
            transform_.quateRotation_.z = degreesToRadians(rotationDegrees[2]);
        }
        ImGui::PopStyleColor();
        ImGui::NextColumn();

        // 大きさ
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
        ImGui::Text("大きさ");
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.3f, 0.2f, 0.5f));
        ImGui::DragFloat3("##大きさ", &transform_.scale_.x, 0.1f, 0.0f);
        ImGui::PopStyleColor();

        ImGui::Columns(1);
        ImGui::Separator();

        // 可視性フラグ
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::Checkbox("表示", &isVisible_);
        ImGui::PopStyleColor();
    }

    if (ImGui::CollapsingHeader("パーティクル設定")) {
        ImGui::DragFloat3("速度最小", &velocityMin_.x, 0.1f);
        ImGui::DragFloat3("速度最大", &velocityMax_.x, 0.1f);
        ImGui::DragFloat3("スケール最小", &scaleMin_.x, 0.01f);
        ImGui::DragFloat3("スケール最大", &scaleMax_.x, 0.01f);
        ImGui::DragFloat("寿命最小", &lifeTimeMin_, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("寿命最大", &lifeTimeMax_, 0.1f, 0.1f, 10.0f);

        ImGui::ColorEdit4("開始色", &startColor_.x);
        ImGui::ColorEdit4("終了色", &endColor_.x);
    }

    if (ImGui::CollapsingHeader("グループ管理")) {
        // アタッチされたグループの表示
        ImGui::Text("アタッチされたグループ:");
        for (size_t i = 0; i < attachedGroups_.size(); ++i) {
            ImGui::Text("- %s", attachedGroups_[i].c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton(("削除##" + std::to_string(i)).c_str())) {
                DetachGroup(attachedGroups_[i]);
                break;
            }
        }

        // 新しいグループの追加
        static char newGroupName[256] = "";
        ImGui::InputText("新規グループ名", newGroupName, sizeof(newGroupName));
        if (ImGui::Button("グループ追加")) {
            if (strlen(newGroupName) > 0) {
                AttachGroup(std::string(newGroupName));
                memset(newGroupName, 0, sizeof(newGroupName));
            }
        }
    }
}

void ParticleCSEmitter::LoadFromJson() {
    if (!datas_)
        return;

    // 基本設定のロード
    isAuto_ = datas_->Load("isAuto", false);
    isActive_ = datas_->Load("isActive", true);
    isVisible_ = datas_->Load("isVisible", true);
    emitFrequency_ = datas_->Load("emitFrequency", 1.0f / 60.0f);
    emitCount_ = datas_->Load("emitCount", static_cast<uint32_t>(10));

    // パーティクル設定
    velocityMin_.x = datas_->Load("velocityMinX", -1.0f);
    velocityMin_.y = datas_->Load("velocityMinY", -1.0f);
    velocityMin_.z = datas_->Load("velocityMinZ", -1.0f);
    velocityMax_.x = datas_->Load("velocityMaxX", 1.0f);
    velocityMax_.y = datas_->Load("velocityMaxY", 1.0f);
    velocityMax_.z = datas_->Load("velocityMaxZ", 1.0f);

    scaleMin_.x = datas_->Load("scaleMinX", 0.5f);
    scaleMin_.y = datas_->Load("scaleMinY", 0.5f);
    scaleMin_.z = datas_->Load("scaleMinZ", 0.5f);
    scaleMax_.x = datas_->Load("scaleMaxX", 1.0f);
    scaleMax_.y = datas_->Load("scaleMaxY", 1.0f);
    scaleMax_.z = datas_->Load("scaleMaxZ", 1.0f);

    lifeTimeMin_ = datas_->Load("lifeTimeMin", 1.0f);
    lifeTimeMax_ = datas_->Load("lifeTimeMax", 3.0f);

    startColor_.x = datas_->Load("startColorR", 1.0f);
    startColor_.y = datas_->Load("startColorG", 1.0f);
    startColor_.z = datas_->Load("startColorB", 1.0f);
    startColor_.w = datas_->Load("startColorA", 1.0f);

    endColor_.x = datas_->Load("endColorR", 1.0f);
    endColor_.y = datas_->Load("endColorG", 1.0f);
    endColor_.z = datas_->Load("endColorB", 1.0f);
    endColor_.w = datas_->Load("endColorA", 0.0f);
}

void ParticleCSEmitter::SaveToJson() {
    if (!datas_)
        return;

    // 基本設定の保存
    datas_->Save("isAuto", isAuto_);
    datas_->Save("isActive", isActive_);
    datas_->Save("isVisible", isVisible_);
    datas_->Save("emitFrequency", emitFrequency_);
    datas_->Save("emitCount", emitCount_);

    // パーティクル設定
    datas_->Save("velocityMinX", velocityMin_.x);
    datas_->Save("velocityMinY", velocityMin_.y);
    datas_->Save("velocityMinZ", velocityMin_.z);
    datas_->Save("velocityMaxX", velocityMax_.x);
    datas_->Save("velocityMaxY", velocityMax_.y);
    datas_->Save("velocityMaxZ", velocityMax_.z);

    datas_->Save("scaleMinX", scaleMin_.x);
    datas_->Save("scaleMinY", scaleMin_.y);
    datas_->Save("scaleMinZ", scaleMin_.z);
    datas_->Save("scaleMaxX", scaleMax_.x);
    datas_->Save("scaleMaxY", scaleMax_.y);
    datas_->Save("scaleMaxZ", scaleMax_.z);

    datas_->Save("lifeTimeMin", lifeTimeMin_);
    datas_->Save("lifeTimeMax", lifeTimeMax_);

    datas_->Save("startColorR", startColor_.x);
    datas_->Save("startColorG", startColor_.y);
    datas_->Save("startColorB", startColor_.z);
    datas_->Save("startColorA", startColor_.w);

    datas_->Save("endColorR", endColor_.x);
    datas_->Save("endColorG", endColor_.y);
    datas_->Save("endColorB", endColor_.z);
    datas_->Save("endColorA", endColor_.w);
}

std::unique_ptr<ParticleCSEmitter> ParticleCSEmitter::Clone() const {
    auto clone = std::make_unique<ParticleCSEmitter>();

    // 基本プロパティのコピー
    clone->name_ = name_ + "_Clone";
    clone->transform_ = transform_;
    clone->isAuto_ = isAuto_;
    clone->isActive_ = isActive_;
    clone->isVisible_ = isVisible_;
    clone->emitFrequency_ = emitFrequency_;
    clone->emitCount_ = emitCount_;

    // パーティクル設定のコピー
    clone->velocityMin_ = velocityMin_;
    clone->velocityMax_ = velocityMax_;
    clone->scaleMin_ = scaleMin_;
    clone->scaleMax_ = scaleMax_;
    clone->lifeTimeMin_ = lifeTimeMin_;
    clone->lifeTimeMax_ = lifeTimeMax_;
    clone->startColor_ = startColor_;
    clone->endColor_ = endColor_;
    clone->colorMode_ = colorMode_;

    // アタッチされたグループのコピー
    clone->attachedGroups_ = attachedGroups_;

    return clone;
}