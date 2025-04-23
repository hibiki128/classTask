#pragma once
#include <ModelStructs.h>
#include <ParticleCommon.h>
#include <WorldTransform.h>
#include <list>
class ParticleGroup {
  public:
    void Initialize();

    void Update();

    ParticleGroupData CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath = {});

    const std::string GetGroupName() { return particleGroupData_.groupName; }
    uint32_t GetMaxInstance() { return kNumMaxInstance; }
    ParticleGroupData& GetParticleGroupData() { return particleGroupData_; }
    std::string &GetTexturePath() { return particleGroupData_.material.textureFilePath; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetmaterialResource() { return materialResource; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexResource() { return vertexResource; }
    D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() { return vertexBufferView; }
    ModelData GetModelData() { return modelData; }
    MaterialData GetMaterialData() { return modelData.material; }

  private:
    void CreateVartexData(const std::string &filename);
    void CreateMaterial();

    /// <summary>
    /// .mtlファイルの読み取り
    /// </summary>
    /// <param name="directoryPath"></param>
    /// <param name="filename"></param>
    /// <returns></returns>
    static MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    ///  .objファイルの読み取り
    /// </summary>
    /// <param name="directoryPath"></param>
    /// <param name="filename"></param>
    /// <returns></returns>
    static ModelData LoadObjFile(const std::string &directoryPath, const std::string &filename);

  private:
    static std::unordered_map<std::string, ModelData> modelCache;
    static const uint32_t kNumMaxInstance = 10000; // 最大インスタンス数の制限

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    // バッファリソース内のデータを指すポインタ
    VertexData *vertexData = nullptr;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    // バッファリソース内のデータを指すポインタ
    Material *materialData = nullptr;

    ModelData modelData;
    ParticleGroupData particleGroupData_;


};
