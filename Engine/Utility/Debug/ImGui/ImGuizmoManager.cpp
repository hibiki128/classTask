#define NOMINMAX
#ifdef _DEBUG
#include "ImGuizmoManager.h"
#include "Input.h"
#include <Transform/WorldTransform.h>

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
    // メインウィンドウのスタイル設定
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.8f, 0.3f));

    ImGui::Begin("トランスフォームマネージャ", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    BaseObject *targetTransform = nullptr;

    // === オブジェクト選択セクション ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
    ImGui::Text("オブジェクト選択");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (!transformMap.empty()) {
        std::vector<const char *> names;
        int currentIndex = 0;
        int i = 0;
        for (const auto &pair : transformMap) {
            names.push_back(pair.first.c_str());
            if (pair.first == selectedName)
                currentIndex = i;
            i++;
        }

        ImGui::PushItemWidth(200);
        if (ImGui::Combo("##ObjectSelector", &currentIndex, names.data(), static_cast<int>(names.size()))) {
            selectedName = names[currentIndex];
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("更新"))
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("リフレッシュ");

        // 選択されたオブジェクト情報を表示
        if (!selectedName.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f));
            ImGui::Text("選択中: %s", selectedName.c_str());
            ImGui::PopStyleColor();
        }

        targetTransform = GetSelectedTarget();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("Transformが登録されていません。");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // === 操作モードセクション ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
    ImGui::Text("操作モード");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // 操作タイプ選択
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));

    if (ImGui::RadioButton("位置", currentOperation == ImGuizmo::TRANSLATE))
        currentOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("オブジェクトを移動します");

    ImGui::SameLine();
    if (ImGui::RadioButton("回転", currentOperation == ImGuizmo::ROTATE))
        currentOperation = ImGuizmo::ROTATE;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("オブジェクトを回転します");

    ImGui::SameLine();
    if (ImGui::RadioButton("大きさ", currentOperation == ImGuizmo::SCALE))
        currentOperation = ImGuizmo::SCALE;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("オブジェクトのサイズを変更します");

    ImGui::PopStyleColor(2);

    ImGui::Spacing();

    // 座標系選択
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.6f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.8f, 1.0f));

    if (ImGui::RadioButton("ローカル", currentMode == ImGuizmo::LOCAL))
        currentMode = ImGuizmo::LOCAL;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("オブジェクトのローカル座標系で操作");

    ImGui::SameLine();
    if (ImGui::RadioButton("ワールド", currentMode == ImGuizmo::WORLD))
        currentMode = ImGuizmo::WORLD;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ワールド座標系で操作");

    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::Spacing();

    // === オブジェクト詳細セクション ===
    if (targetTransform) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("オブジェクト詳細");
        ImGui::PopStyleColor();
        ImGui::Separator();

        targetTransform->ImGui();
    }

    ImGui::PopStyleColor(); // Header color
    ImGui::PopStyleVar(2);  // Window padding and item spacing
    ImGui::End();

    if (!viewProjection)
        return;

    // === マウスピック処理 ===
    float mouseX = Input::GetInstance()->GetMousePos().x;
    float mouseY = Input::GetInstance()->GetMousePos().y;
    bool leftClicked = Input::GetInstance()->IsTriggerMouse(0);

    static std::string lastPickedName;
    static bool gizmoActive = false;

    // シーンウィンドウ内か判定
    if (mouseX >= scenePosition.x && mouseX <= scenePosition.x + sceneSize.x &&
        mouseY >= scenePosition.y && mouseY <= scenePosition.y + sceneSize.y) {

        if (leftClicked) {
            // 画面座標をNDC(-1~1)に変換
            float ndcX = 2.0f * (mouseX - scenePosition.x) / sceneSize.x - 1.0f;
            float ndcY = 1.0f - 2.0f * (mouseY - scenePosition.y) / sceneSize.y;

            // 最も近いオブジェクトを1つだけ判定（AABBの2D投影で簡易判定）
            float minDistSq = std::numeric_limits<float>::max();
            std::string pickedName;
            for (const auto &pair : transformMap) {
                BaseObject *obj = pair.second;
                Vector3 pos = obj->GetLocalPosition();

                // ワールド座標をスクリーン座標に変換
                Vector3 screenPos;
                {
                    // ビュー射影変換
                    Vector3 v = pos;
                    float x = v.x * viewProjection->matView_.m[0][0] + v.y * viewProjection->matView_.m[1][0] + v.z * viewProjection->matView_.m[2][0] + viewProjection->matView_.m[3][0];
                    float y = v.x * viewProjection->matView_.m[0][1] + v.y * viewProjection->matView_.m[1][1] + v.z * viewProjection->matView_.m[2][1] + viewProjection->matView_.m[3][1];
                    float z = v.x * viewProjection->matView_.m[0][2] + v.y * viewProjection->matView_.m[1][2] + v.z * viewProjection->matView_.m[2][2] + viewProjection->matView_.m[3][2];
                    float w = v.x * viewProjection->matView_.m[0][3] + v.y * viewProjection->matView_.m[1][3] + v.z * viewProjection->matView_.m[2][3] + viewProjection->matView_.m[3][3];
                    // 射影
                    float px = x * viewProjection->matProjection_.m[0][0] + y * viewProjection->matProjection_.m[1][0] + z * viewProjection->matProjection_.m[2][0] + w * viewProjection->matProjection_.m[3][0];
                    float py = x * viewProjection->matProjection_.m[0][1] + y * viewProjection->matProjection_.m[1][1] + z * viewProjection->matProjection_.m[2][1] + w * viewProjection->matProjection_.m[3][1];
                    float pz = x * viewProjection->matProjection_.m[0][2] + y * viewProjection->matProjection_.m[1][2] + z * viewProjection->matProjection_.m[2][2] + w * viewProjection->matProjection_.m[3][2];
                    float pw = x * viewProjection->matProjection_.m[0][3] + y * viewProjection->matProjection_.m[1][3] + z * viewProjection->matProjection_.m[2][3] + w * viewProjection->matProjection_.m[3][3];
                    if (pw != 0.0f) {
                        screenPos.x = px / pw;
                        screenPos.y = py / pw;
                        screenPos.z = pz / pw;
                    } else {
                        screenPos.x = screenPos.y = screenPos.z = 0.0f;
                    }
                }
                // NDC→ウィンドウ座標
                float sx = scenePosition.x + (screenPos.x * 0.5f + 0.5f) * sceneSize.x;
                float sy = scenePosition.y + (0.5f - screenPos.y * 0.5f) * sceneSize.y;

                float dx = mouseX - sx;
                float dy = mouseY - sy;
                float distSq = dx * dx + dy * dy;

                // 半径はGetRadius()で取得、2D投影で十分
                float radius = obj->GetLocalScale().x;
                float screenRadius = radius * 100.0f; // 適当なスケール（必要に応じて調整）

                if (distSq < screenRadius * screenRadius && distSq < minDistSq) {
                    minDistSq = distSq;
                    pickedName = pair.first;
                }
            }
            if (!pickedName.empty()) {
                selectedName = pickedName;
                gizmoActive = true;
                lastPickedName = pickedName;
            } else {
                gizmoActive = false;
            }
        }
    }

    // === ImGuizmoの処理は必要条件を満たすときのみ ===
    targetTransform = GetSelectedTarget();
    if (!targetTransform)
        return;

    // ギズモがアクティブな時のみImGuizmoを有効化
    if (!gizmoActive)
        return;

    float matrix[16];
    ConvertMatrix4x4ToFloat16(targetTransform->GetWorldTransform()->matWorld_, matrix);

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
        ConvertFloat16ToMatrix4x4(matrix, targetTransform->GetWorldTransform()->matWorld_);
        DecomposeMatrix(targetTransform->GetWorldTransform());
        targetTransform->GetWorldTransform()->TransferMatrix();
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

#endif // _DEBUG