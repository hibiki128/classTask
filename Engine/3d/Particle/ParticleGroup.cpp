#include "ParticleGroup.h"
#include "Srv/SrvManager.h"
#include "fstream"
#include <Texture/TextureManager.h>

std::unordered_map<std::string, ModelData> ParticleGroup::modelCache;

void ParticleGroup::Initialize() {
}

void ParticleGroup::Update() {
}

ParticleGroupData ParticleGroup::CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath) {
    particleGroupData_.groupName = groupName;
    particleGroupData_.material.modelFilePath = filename;
    CreateVartexData(filename);
    if (texturePath.empty()) {
        particleGroupData_.material.textureFilePath = modelData.material.textureFilePath;
    } else {
        particleGroupData_.material.textureFilePath = texturePath;
    }
    TextureManager::GetInstance()->LoadTexture(particleGroupData_.material.textureFilePath);
    particleGroupData_.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(particleGroupData_.material.textureFilePath);
    particleGroupData_.instancingResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
    particleGroupData_.instancingSRVIndex = SrvManager::GetInstance()->Allocate() + 1;
    particleGroupData_.instancingResource->Map(0, nullptr, reinterpret_cast<void **>(&particleGroupData_.instancingData));

    SrvManager::GetInstance()->CreateSRVforStructuredBuffer(particleGroupData_.instancingSRVIndex, particleGroupData_.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

    CreateMaterial();
    particleGroupData_.instanceCount = 0;
    return particleGroupData_;
}

void ParticleGroup::CreateVartexData(const std::string &filename) {
    modelData = LoadObjFile("resources/models/", filename);

    // 頂点リソースを作る
    vertexResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
    // 頂点バッファビューを作成する
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();            // リソースの先頭アドレスから使う
    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData);                                 // 1頂点当たりのサイズ

    // 頂点リソースにデータを書き込む
    vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData)); // 書き込むためのアドレスを取得
    std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void ParticleGroup::CreateMaterial() {
    // Sprite用のマテリアルリソースをつくる
    materialResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Material));
    // 書き込むためのアドレスを取得
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
    // 色の設定
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    // Lightingの設定
    materialData->uvTransform = MakeIdentity4x4();
}

MaterialData ParticleGroup::LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
    MaterialData materialData;                          // 構築するMaterialData
    std::string line;                                   // ファイルから読んだ1行を格納するもの
    std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
    assert(file.is_open());                             // 開けなかったら止める
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        // identifierに応じた処理
        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            // 連結してファイルパスにする
            materialData.textureFilePath = textureFilename;
        }
    }

    // テクスチャが張られていない場合の処理
    if (materialData.textureFilePath.empty()) {
        materialData.textureFilePath = "debug/white1x1.png";
    }

    return materialData;
}

ModelData ParticleGroup::LoadObjFile(const std::string &directoryPath, const std::string &filename) {
    std::string fullPath = directoryPath + filename;

    // キャッシュを確認して、既に読み込まれている場合はそれを返す
    auto it = modelCache.find(fullPath);
    if (it != modelCache.end()) {
        return it->second;
    }

    ModelData modelData;
    std::vector<Vector4> positions; // 位置
    std::vector<Vector2> texcoords; // テクスチャ座標
    std::string line;               // ファイルから読んだ1行目を格納するもの

    // ファイル名からフォルダ部分を取得
    std::string folderPath;
    size_t lastSlashPos = filename.find_last_of("/\\");
    if (lastSlashPos != std::string::npos) {
        folderPath = filename.substr(0, lastSlashPos);
    }

    std::ifstream file(fullPath); // ファイルを開く
    assert(file.is_open());       // ファイルが開けなかったら停止

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "v") {
            Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.x *= -1.0f;
            position.w = 1.0f;
            positions.push_back(position);
        } else if (identifier == "vt") {
            Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoord.y = 1.0f - texcoord.y;
            texcoords.push_back(texcoord);
        } else if (identifier == "f") {
            VertexData triangle[3];
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefinition;
                s >> vertexDefinition;
                std::istringstream v(vertexDefinition);
                uint32_t elementIndices[3];
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(v, index, '/');
                    elementIndices[element] = std::stoi(index);
                }
                Vector4 position = positions[elementIndices[0] - 1];
                Vector2 texcoord = texcoords[elementIndices[1] - 1];
                VertexData vertex = {position, texcoord};
                modelData.vertices.push_back(vertex);
                triangle[faceVertex] = {position, texcoord};
            }
        } else if (identifier == "mtllib") {
            std::string materialFilename;
            s >> materialFilename;
            if (!folderPath.empty()) {
                modelData.material = LoadMaterialTemplateFile(directoryPath + folderPath, materialFilename);
            } else {
                modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
            }
        }
    }

    // キャッシュに保存
    modelCache[fullPath] = modelData;

    return modelData;
}
