#include "ParticleGroupManager.h"

ParticleGroupManager *ParticleGroupManager::instance = nullptr;

ParticleGroupManager *ParticleGroupManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleGroupManager();
    }
    return instance;
}

void ParticleGroupManager::Initialize() {
    // パーティクルグループのJSONファイルが格納されているディレクトリを走査
    const std::string directoryPath = "resources/jsons/ParticleGroup/";

    // ディレクトリが存在しない場合は何もしない
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return;
    }

    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                json jsonData;
                file >> jsonData;
                std::string groupName = jsonData["groupName"];
                std::string filename = jsonData["modelfilePath"];
                std::string texturePath = jsonData["textrueName"];
                CreateParticleGroup(groupName, filename, texturePath);
                file.close();
            }
        }
    }
}



void ParticleGroupManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void ParticleGroupManager::AddParticleGroup(std::unique_ptr<ParticleGroup> particleGroup) {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleGroup", particleGroup->GetGroupName());
    data->Save("groupName", particleGroup->GetGroupName());
    data->Save("textrueName", particleGroup->GetParticleGroupData().material.textureFilePath);
    data->Save("modelfilePath", particleGroup->GetParticleGroupData().material.modelFilePath);
    particleGroups_.emplace_back(std::move(particleGroup));
}

void ParticleGroupManager::CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath) {
    auto particleGroup = std::make_unique<ParticleGroup>();
    particleGroup->CreateParticleGroup(groupName, filename, texturePath);
    AddParticleGroup(std::move(particleGroup));
}
