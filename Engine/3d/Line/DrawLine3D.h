#pragma once
#include "PipeLine/PipeLineManager.h"
#include "ViewProjection/ViewProjection.h"
#include <Matrix4x4.h>
#include <Vector3.h>
#include <Vector4.h>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl/client.h>

using namespace Microsoft::WRL;

class DrawLine3D {
  public:
    static DrawLine3D *instance;

    DrawLine3D() = default;
    ~DrawLine3D() = default;
    DrawLine3D(DrawLine3D &) = delete;
    DrawLine3D &operator=(DrawLine3D &) = delete;

  public:
    static const UINT kMaxLineCount = 65536;
    static const UINT kVertexCountLine = 2;
    static const UINT kIndexCountLine = 0;

    struct VertexPosColor {
        Vector3 pos;
        Vector4 color;
    };

    struct LineData {
        ComPtr<ID3D12Resource> vertBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vbView{};
        D3D12_INDEX_BUFFER_VIEW ibView{};
        VertexPosColor *vertMap = nullptr;
        uint16_t *indexMap = nullptr;
    };

    std::unique_ptr<LineData> CreateMesh(UINT vertexCount, UINT indexCount);

    static DrawLine3D *GetInstance();

    void Initialize();

    void Finalize();

    void SetPoints(const Vector3 &p1, const Vector3 &p2, const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f});

    void Reset();

    void Draw(const ViewProjection &viewProjection);

    void DrawGrid(float y, int division, float size, Vector4 color = {0.5f, 0.5f, 0.5f, 1.0f});

  private:
    void CreateMeshes();

    void CreateResource();

  private:
    std::unique_ptr<LineData> line_;

    uint32_t indexLine_ = 0;

    DirectXCommon *dxCommon = nullptr;

    PipeLineManager *psoManager_ = nullptr;

    struct CBuffer {
        Matrix4x4 viewProject;
    };

    ComPtr<ID3D12Resource> cBufferResource_ = nullptr;
    CBuffer *cBufferData_ = nullptr;
};