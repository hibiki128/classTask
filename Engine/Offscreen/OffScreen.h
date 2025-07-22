#pragma once
#include "externals/nlohmann/json.hpp"
#include "myMath.h"
#include "wrl.h"
#include <d3d12.h>
#include <type/Matrix4x4.h>
#include <type/Vector2.h>
#include <vector>

#include <Graphics/PipeLine/PipeLineManager.h>
#include <Graphics/Srv/SrvManager.h>
#include"Data/DataHandler.h"

class DirectXCommon;

// エフェクトの設定を保存する構造体
struct PostEffectSettings {
    ShaderMode shaderMode = ShaderMode::kNone;
    bool enabled = false;
};

class OffScreen {
  public:
    void Initialize();
    void CreateFinalResultTexture();
    void Draw();
    void DrawToFinalResult();
    void CopyFinalResultToBackBuffer();
    void Setting();
    void SetProjection(Matrix4x4 projectionMatrix) { projectionInverse_ = projectionMatrix; }

    // 新しいメソッド
    void AddEffect(ShaderMode mode);
    void RemoveEffect(int index);
    void SetEffectEnabled(int index, bool enabled);
    void MoveEffectUp(int index);
    void MoveEffectDown(int index);

    void SaveData();
    void LoadData();

      uint32_t GetFinalResultSrvIndex() const { return finalResultSrvIndex_; }

  private:
    void CreateSmooth();
    void CreateGauss();
    void CreateVignette();
    void CreateDepth();
    void CreateRadial();
    void CreateCinematic();

    // 新しいメソッド
    void CreatePingPongBuffers();
    void DrawSingleEffect(ShaderMode mode, bool isFirstInput, int inputPingPongIndex, int outputRtvIndex);

    
    // データハンドラー関連メソッド
    void InitializeDataHandler();
    void SaveEffectChain();
    void LoadEffectChain();
    void SaveEffectParameters();
    void LoadEffectParameters();

  private:
    DirectXCommon *dxCommon;
    SrvManager *srvManager_;
    PipeLineManager *psoManager_ = nullptr;

    // エフェクトチェーン管理
    std::vector<PostEffectSettings> effectChain_;
    int currentPingPongBuffer_ = 0;

    // ピンポンバッファ用リソース
    static const int kPingPongBufferCount = 2;
    Microsoft::WRL::ComPtr<ID3D12Resource> pingPongResources_[kPingPongBufferCount];
    uint32_t pingPongSrvIndices_[kPingPongBufferCount];
    D3D12_CPU_DESCRIPTOR_HANDLE pingPongRtvHandles_[kPingPongBufferCount];
    D3D12_CPU_DESCRIPTOR_HANDLE pingPongSrvHandlesCPU_[kPingPongBufferCount];
    D3D12_GPU_DESCRIPTOR_HANDLE pingPongSrvHandlesGPU_[kPingPongBufferCount];

     Microsoft::WRL::ComPtr<ID3D12Resource> finalResultResource_;
    uint32_t finalResultSrvIndex_;
    D3D12_CPU_DESCRIPTOR_HANDLE finalResultRtvHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE finalResultSrvHandleCPU_;
    D3D12_GPU_DESCRIPTOR_HANDLE finalResultSrvHandleGPU_;

    using json = nlohmann::json;

    struct KernelSettings {
        int kernelSize;
    };

    struct GaussianParams {
        int kernelSize;
        float sigma;
    };

    struct VignetteParameter {
        float vignetteStrength;
        float vignetteRadius;
        float vignetteExponent;
        float padding;
        Vector2 vignetteCenter;
    };

    struct Depth {
        Matrix4x4 projectionInverse;
        int kernelSize;
    };

    struct RadialBlur {
        Vector2 kCenter;
        float kBlurWidth;
    };

    struct Cinematic {
        Vector2 iResolution;
        float contrast;
        float saturation;
        float brightness;
    };

    // バッファリソース（既存のまま）
    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;
    VignetteParameter *vignetteData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> smoothResource;
    KernelSettings *smoothData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianResouce;
    GaussianParams *gaussianData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthResouce;
    Depth *depthData = nullptr;

    Matrix4x4 projectionInverse_;

    Microsoft::WRL::ComPtr<ID3D12Resource> radialResource;
    RadialBlur *radialData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> cinematicResource;
    Cinematic *cinematicData = nullptr;

     // データハンドラー
    std::unique_ptr<DataHandler> dataHandler_;

};