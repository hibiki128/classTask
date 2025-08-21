#include "ParticleCSGroupManager.h"
#include "Graphics/Model/ModelManager.h"
#include "Graphics/Texture/TextureManager.h"
#include <fstream>

ParticleCSGroupManager *ParticleCSGroupManager::instance = nullptr;

ParticleCSGroupManager *ParticleCSGroupManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleCSGroupManager();
    }
    return instance;
}

void ParticleCSGroupManager::Initialize() {
    const std::string directoryPath = "resources/jsons/ParticleCSGroup/";

    // ディレクトリが存在しない場合は何もしない
    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        return;
    }

    // JSONファイルからグループ情報を読み込み
    for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            LoadGroupFromJson(entry.path().string());
        }
    }
}

void ParticleCSGroupManager::Finalize() {
    groups_.clear();
    delete instance_;
    instance_ = nullptr;
}

void ParticleCSGroupManager::CreateModelGroup(const std::string &groupName, const std::string &modelPath, const std::string &texturePath) {
    // 既に存在する場合は作成しない
    if (HasGroup(groupName)) {
        return;
    }

    auto group = std::make_unique<ParticleCSGroup>();
    group->InitializeWithModel(groupName, modelPath, texturePath);

    groups_[groupName] = std::move(group);

    // JSONファイルに保存
    SaveGroupToJson(groupName);
}

void ParticleCSGroupManager::CreatePrimitiveGroup(const std::string &groupName, PrimitiveType primitiveType, const std::string &texturePath) {
    // 既に存在する場合は作成しない
    if (HasGroup(groupName)) {
        return;
    }

    auto group = std::make_unique<ParticleCSGroup>();
    group->InitializeWithPrimitive(groupName, primitiveType, texturePath);

    groups_[groupName] = std::move(group);

    // JSONファイルに保存
    SaveGroupToJson(groupName);
}

void ParticleCSGroupManager::RemoveGroup(const std::string &groupName) {
    auto it = groups_.find(groupName);
    if (it != groups_.end()) {
        groups_.erase(it);

        // JSONファイルも削除
        std::string jsonPath = "resources/jsons/ParticleCSGroup/" + groupName + ".json";
        if (std::filesystem::exists(jsonPath)) {
            std::filesystem::remove(jsonPath);
        }
    }
}

std::shared_ptr<ParticleCSGroup> ParticleCSGroupManager::GetGroup(const std::string &groupName) {
    auto it = groups_.find(groupName);
    if (it != groups_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ParticleCSGroupManager::HasGroup(const std::string &groupName) const {
    return groups_.find(groupName) != groups_.end();
}

std::vector<std::string> ParticleCSGroupManager::GetGroupNames() const {
    std::vector<std::string> names;
    for (const auto &pair : groups_) {
        names.push_back(pair.first);
    }
    return names;
}

void ParticleCSGroupManager::LoadGroupFromJson(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    // グループ名の取得
    if (!jsonData.contains("groupName") || !jsonData["groupName"].is_string()) {
        return;
    }

    std::string groupName = jsonData["groupName"];
    std::string texturePath = jsonData.value("texturePath", "");

    // モデルパスが存在する場合
    if (jsonData.contains("modelPath") && !jsonData["modelPath"].empty()) {
        std::string modelPath = jsonData["modelPath"];
        CreateModelGroup(groupName, modelPath, texturePath);
    }
    // プリミティブタイプが存在する場合
    else if (jsonData.contains("primitiveType")) {
        int primitiveValue = jsonData["primitiveType"].get<int>();
        if (primitiveValue >= 0) {
            PrimitiveType type = static_cast<PrimitiveType>(primitiveValue);
            CreatePrimitiveGroup(groupName, type, texturePath);
        }
    }
}

void ParticleCSGroupManager::SaveGroupToJson(const std::string &groupName) {
    auto group = GetGroup(groupName);
    if (!group) {
        return;
    }

    nlohmann::json jsonData;
    jsonData["groupName"] = groupName;
    jsonData["texturePath"] = group->GetTexturePath();

    if (!group->GetModelPath().empty()) {
        jsonData["modelPath"] = group->GetModelPath();
        jsonData["primitiveType"] = -1; // モデルの場合は-1
    } else {
        jsonData["modelPath"] = "";
        jsonData["primitiveType"] = static_cast<int>(group->GetPrimitiveType());
    }

    // ディレクトリが存在しない場合は作成
    std::string dirPath = "resources/jsons/ParticleCSGroup/";
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
    }

    // ファイルに保存
    std::string filePath = dirPath + groupName + ".json";
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << jsonData.dump(4);
    }
}

size_t ParticleCSGroupManager::GetTotalParticleCount() const {
    size_t totalCount = 0;
    for (const auto &pair : groups_) {
        totalCount += pair.second->GetActiveParticleCount();
    }
    return totalCount;
}

void ParticleCSGroupManager::ResetAllParticles() {
    for (auto &pair : groups_) {
        pair.second->ResetParticles();
    }
}