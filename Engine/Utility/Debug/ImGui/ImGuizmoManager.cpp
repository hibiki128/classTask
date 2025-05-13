#include "ImGuizmoManager.h"

ImGuizmoManager *ImGuizmoManager::instance = nullptr;

ImGuizmoManager *ImGuizmoManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ImGuizmoManager;
    }
    return instance;
}

void ImGuizmoManager::Finalize() {
    transformMap.clear();
    selectedName.clear();
}

void ImGuizmoManager::BeginFrame() {
    ImGuizmo::BeginFrame();
}

void ImGuizmoManager::SetViewProjection(ViewProjection *vp) {
    viewProjection = vp;
}

void ImGuizmoManager::AddTarget(const std::string &name, BaseObject *transform) {
    transformMap[name] = transform;

    // 初期選択が未設定の場合、最初に追加されたものを自動選択
    if (selectedName.empty()) {
        selectedName = name;
    }
}

BaseObject *ImGuizmoManager::GetSelectedTarget() {
    auto it = transformMap.find(selectedName);
    return (it != transformMap.end()) ? it->second : nullptr;
}

void ImGuizmoManager::Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    // === ウィンドウは常に表示 ===
    ImGui::Begin("トランスフォームエディター");

    BaseObject *targetTransform = nullptr;

    if (!transformMap.empty()) {
        // === Comboで対象選択 ===
        std::vector<const char *> names;
        int currentIndex = 0;
        int i = 0;
        for (const auto &pair : transformMap) {
            names.push_back(pair.first.c_str());
            if (pair.first == selectedName)
                currentIndex = i;
            i++;
        }

        if (ImGui::Combo("選択", &currentIndex, names.data(), static_cast<int>(names.size()))) {
            // 新しい選択を適用
            selectedName = names[currentIndex];
        }

        // 選択中のTransformを取得
        targetTransform = GetSelectedTarget();
    } else {
        ImGui::Text("Transformが登録されていません。");
    }

    // === 操作モード切り替え ===
    if (ImGui::RadioButton("位置", currentOperation == ImGuizmo::TRANSLATE))
        currentOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("回転", currentOperation == ImGuizmo::ROTATE))
        currentOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("大きさ", currentOperation == ImGuizmo::SCALE))
        currentOperation = ImGuizmo::SCALE;

    ImGui::Separator();

    if (ImGui::RadioButton("ローカル", currentMode == ImGuizmo::LOCAL))
        currentMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("ワールド", currentMode == ImGuizmo::WORLD))
        currentMode = ImGuizmo::WORLD;

    ImGui::Separator();

    // === 選択中のTransformを表示・操作 ===
    if (targetTransform) {
        targetTransform->DebugImGui();
    }

    ImGui::End(); // ウィンドウは必ず閉じる

    // === ImGuizmoの処理は必要条件を満たすときのみ ===
    if (!viewProjection || !targetTransform)
        return;

    // === ImGuizmoによる操作 ===
    float matrix[16];
    ConvertMatrix4x4ToFloat16(targetTransform->GetWorldTransform().matWorld_, matrix);

    float viewMatrix[16];
    float projMatrix[16];
    ConvertMatrix4x4ToFloat16(viewProjection->matView_, viewMatrix);
    ConvertMatrix4x4ToFloat16(viewProjection->matProjection_, projMatrix);

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(scenePosition.x, scenePosition.y, sceneSize.x, sceneSize.y);

    bool manipulated = ImGuizmo::Manipulate(
        viewMatrix,
        projMatrix,
        currentOperation,
        currentMode,
        matrix);

    if (manipulated) {
        // マトリックス更新
        ConvertFloat16ToMatrix4x4(matrix, targetTransform->GetWorldTransform().matWorld_);
        DecomposeMatrix(&targetTransform->GetWorldTransform());
        targetTransform->GetWorldTransform().TransferMatrix(); // 定数バッファへ転送
    }
}

// 行列 → float[16]
void ImGuizmoManager::ConvertMatrix4x4ToFloat16(const Matrix4x4 &matrix, float *outMatrix) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            outMatrix[i * 4 + j] = matrix.m[i][j];
}

// float[16] → 行列
void ImGuizmoManager::ConvertFloat16ToMatrix4x4(const float *inMatrix, Matrix4x4 &outMatrix) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            outMatrix.m[i][j] = inMatrix[i * 4 + j];
}

// ImGuizmoを使って行列からTRSを分解
void ImGuizmoManager::DecomposeMatrix(WorldTransform *transform) {
    float translation[3], rotation[3], scale[3];
    ImGuizmo::DecomposeMatrixToComponents(
        reinterpret_cast<float *>(&transform->matWorld_.m[0][0]),
        translation, rotation, scale);

    const float DEG_TO_RAD = 0.01745329251f;

    transform->translation_ = {translation[0], translation[1], translation[2]};
    transform->rotation_ = {
        rotation[0] * DEG_TO_RAD,
        rotation[1] * DEG_TO_RAD,
        rotation[2] * DEG_TO_RAD};
    transform->scale_ = {scale[0], scale[1], scale[2]};
}
