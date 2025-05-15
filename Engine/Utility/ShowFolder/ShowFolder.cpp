#include "ShowFolder.h"
#include <vector>
#include <filesystem>
#include <imgui.h>

void ShowTextureFile(std::string &selectedTexturePath) {
    // テクスチャファイル選択
    static std::filesystem::path baseDirTex = "resources/images/";
    static std::filesystem::path currentDirTex = "resources/images";
    static std::string selectedFolderTex = "";
    static std::string selectedFileTex = "";

    // 「戻る」ボタン（テクスチャ用）
    if (currentDirTex != "resources/images") {
        if (ImGui::Button("< 戻る(Tex)")) {
            currentDirTex = currentDirTex.parent_path();
            selectedFolderTex = "";
            selectedFileTex = "";
        }
    }

    // フォルダ一覧
    std::vector<std::string> foldersTex;
    std::vector<std::string> texFiles;

    for (const auto &entry : std::filesystem::directory_iterator(currentDirTex)) {
        if (entry.is_directory()) {
            foldersTex.push_back(entry.path().filename().string());
        } else if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            texFiles.push_back(entry.path().filename().string());
        }
    }

    // フォルダ選択 (クリックで移動)
    if (!foldersTex.empty()) {
        ImGui::Text("フォルダ");
        ImGui::Separator();
        for (auto &folder : foldersTex) {
            std::string folderNameTex = folder + " (Tex)"; // フォルダ名に "(Tex)" を追加
            if (ImGui::Selectable(folderNameTex.c_str(), selectedFolderTex == folder)) {
                selectedFolderTex = folderNameTex;
                currentDirTex = currentDirTex / folder; // フォルダ移動
                selectedFileTex = "";                   // 新しいフォルダを開いたらファイル選択をリセット
            }
            ImGui::Separator();
        }
    }

    // `.png`, `.jpg` テクスチャファイル選択
    if (!texFiles.empty()) {
        ImGui::Text("テクスチャファイル:");
        if (ImGui::BeginCombo("ファイル選択 ", selectedFileTex.empty() ? "なし" : selectedFileTex.c_str())) {
            for (const auto &file : texFiles) {
                bool isSelected = (file == selectedFileTex);
                if (ImGui::Selectable(file.c_str(), isSelected)) {
                    selectedFileTex = file;

                    // `baseDirTex` からの相対パスを取得
                    std::filesystem::path relativePath = (currentDirTex / file).lexically_relative(baseDirTex);

                    // Windowsのバックスラッシュをスラッシュに変換
                    std::string pathStr = relativePath.string();
                    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                    // 選択されたテクスチャパスを設定
                    selectedTexturePath = pathStr;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
}
