#pragma once
#include "externals/nlohmann/json.hpp"
#include "myMath.h"
#include "wrl.h"
#include <d3d12.h>
#include <type/Matrix4x4.h>
#include <type/Vector2.h>
#include <vector>

#include "Data/DataHandler.h"
#include <Graphics/PipeLine/PipeLineManager.h>
#include <Graphics/Srv/SrvManager.h>

class DirectXCommon;

// エフェクトの設定を保存する構造体
struct PostEffectSettings {
    ShaderMode shaderMode = ShaderMode::kNone;
    bool enabled = false;
};

class OffScreen {
  public:
    void Initialize();
    void Draw();
    void Setting();
    void SetProjection(Matrix4x4 projectionMatrix) { projectionInverse_ = projectionMatrix; }

    void SaveData();
    void LoadData();

  private:
    void CreateSmooth();
    void CreateGauss();
    void CreateVignette();
    void CreateDepth();
    void CreateRadial();
    void CreateCinematic();
    void CreateDissolve();

    // データハンドラー関連メソッド
    void InitializeDataHandler();
    void SaveEffectParameters();
    void LoadEffectParameters();

  private:
    DirectXCommon *dxCommon;
    SrvManager *srvManager_;
    PipeLineManager *psoManager_ = nullptr;
    ShaderMode shaderMode_ = ShaderMode::kNone;

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

    struct Dissolve {
        float value;
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

    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveResource;
    Dissolve *dissolveData = nullptr;

    std::string folderPath_ = "debug/noise0.png";

    // データハンドラー
    std::unique_ptr<DataHandler> dataHandler_;
};