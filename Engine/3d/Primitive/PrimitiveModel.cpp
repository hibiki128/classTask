#include "PrimitiveModel.h"
#include <DirectXMath.h>
#include <myMath.h>
using namespace DirectX;

PrimitiveModel *PrimitiveModel::instance = nullptr;

void PrimitiveModel::Initialize() {
    CreateSphere();
    CreatePlane();
    CreateCube();
    CreateCylinder();
    CreateRing();
}

PrimitiveModel *PrimitiveModel::GetInstance() {
    if (instance == nullptr) {
        instance = new PrimitiveModel();
    }
    return instance;
}

void PrimitiveModel::Finalize() {
    delete instance;
    instance = nullptr;
}

void PrimitiveModel::CreateSphere() {
    // Sphereの頂点データ
    PrimitiveData primitiveData{};
    // 球分割数
    const uint32_t kSubdivision = 32;
    const float kLonEvery = std::numbers::pi_v<float> * 2.0f / static_cast<float>(kSubdivision);
    const float kLatEvery = std::numbers::pi_v<float> / static_cast<float>(kSubdivision);

    // 頂点の生成
    for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
        float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
        float sinLat = std::sinf(lat);
        float cosLat = std::cosf(lat);

        for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
            float lon = kLonEvery * lonIndex;
            float sinLon = std::sinf(lon);
            float cosLon = std::cosf(lon);

            VertexData vertex{};
            vertex.position = {cosLat * cosLon, sinLat, cosLat * sinLon, 1.0f};
            vertex.normal = {cosLat * cosLon, sinLat, cosLat * sinLon};
            vertex.texcoord = {static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
                               1.0f - static_cast<float>(latIndex) / static_cast<float>(kSubdivision)};
            primitiveData.vertices.push_back(vertex);
        }
    }

    // インデックスの生成
    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            uint32_t first = latIndex * (kSubdivision + 1) + lonIndex;
            uint32_t second = first + kSubdivision + 1;

            // 三角形1
            primitiveData.indices.push_back(first);
            primitiveData.indices.push_back(second);
            primitiveData.indices.push_back(first + 1);

            // 三角形2
            primitiveData.indices.push_back(second);
            primitiveData.indices.push_back(second + 1);
            primitiveData.indices.push_back(first + 1);
        }
    }

    primitiveData.color = {1.0f, 1.0f, 1.0f, 1.0f};
    primitiveData.uvMatrix = MakeIdentity4x4();

    // コンテナに挿入
    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Sphere, primitiveData));
}

void PrimitiveModel::CreatePlane() {
    // Planeの頂点データ（垂直配置）
    PrimitiveData primitiveData{};

    // 頂点の生成（X軸周りに90度回転 - 垂直配置）
    primitiveData.vertices = {
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, // 左下
        {{1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  // 右下
        {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // 左上
        {{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}    // 右上
    };

    // インデックスの生成
    primitiveData.indices = {
        0, 2, 1, // 三角形1
        1, 2, 3  // 三角形2
    };

    primitiveData.color = {1.0f, 1.0f, 1.0f, 1.0f};
    primitiveData.uvMatrix = MakeIdentity4x4();

    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Plane, primitiveData));
}

void PrimitiveModel::CreateCube() {
    // Cubeの頂点データ
    PrimitiveData primitiveData{};

    // 頂点の生成（8頂点）
    VertexData vertices[8] = {
        // 前面（z = -1）
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
        {{1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},  // 右下
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 左上
        {{1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},   // 右上
        // 背面（z = 1）
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, // 左下
        {{1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  // 右下
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // 左上
        {{1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}    // 右上
    };

    // 各面の頂点を追加
    // 前面（z = -1）
    primitiveData.vertices.push_back(vertices[0]); // 0: 左下
    primitiveData.vertices.push_back(vertices[1]); // 1: 右下
    primitiveData.vertices.push_back(vertices[2]); // 2: 左上
    primitiveData.vertices.push_back(vertices[3]); // 3: 右上

    // 背面（z = 1）
    primitiveData.vertices.push_back(vertices[5]); // 4: 右下
    primitiveData.vertices.push_back(vertices[4]); // 5: 左下
    primitiveData.vertices.push_back(vertices[7]); // 6: 右上
    primitiveData.vertices.push_back(vertices[6]); // 7: 左上

    // 右面（x = 1）
    VertexData rightFace[4] = {
        {{1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, // 8: 左下
        {{1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},  // 9: 右下
        {{1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // 10: 左上
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}    // 11: 右上
    };
    for (int i = 0; i < 4; i++) {
        primitiveData.vertices.push_back(rightFace[i]);
    }

    // 左面（x = -1）
    VertexData leftFace[4] = {
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},  // 12: 左下
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}}, // 13: 右下
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},   // 14: 左上
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}   // 15: 右上
    };
    for (int i = 0; i < 4; i++) {
        primitiveData.vertices.push_back(leftFace[i]);
    }

    // 上面（y = 1）
    VertexData topFace[4] = {
        {{-1.0f, 1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}, // 16: 左下
        {{1.0f, 1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},  // 17: 右下
        {{-1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // 18: 左上
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}    // 19: 右上
    };
    for (int i = 0; i < 4; i++) {
        primitiveData.vertices.push_back(topFace[i]);
    }

    // 下面（y = -1）
    VertexData bottomFace[4] = {
        {{-1.0f, -1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},  // 20: 左下
        {{1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},   // 21: 右下
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, // 22: 左上
        {{1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}   // 23: 右上
    };
    for (int i = 0; i < 4; i++) {
        primitiveData.vertices.push_back(bottomFace[i]);
    }

    // 各面のインデックスを設定（2つの三角形で1つの四角形）
    for (int face = 0; face < 6; face++) {
        uint32_t baseIndex = face * 4;
        // 三角形1
        primitiveData.indices.push_back(baseIndex + 0);
        primitiveData.indices.push_back(baseIndex + 2);
        primitiveData.indices.push_back(baseIndex + 1);
        // 三角形2
        primitiveData.indices.push_back(baseIndex + 1);
        primitiveData.indices.push_back(baseIndex + 2);
        primitiveData.indices.push_back(baseIndex + 3);
    }

    primitiveData.color = {1.0f, 1.0f, 1.0f, 1.0f};
    primitiveData.uvMatrix = MakeIdentity4x4();

    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Cube, primitiveData));
}

void PrimitiveModel::CreateCylinder() {
    // Cylinderの頂点データ
    PrimitiveData primitiveData{};

    const uint32_t kRingDivide = 32;
    const float kRadius = 1.0f;
    const float kHeight = 2.0f;
    const float halfHeight = kHeight / 2.0f;
    const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

    // 頂点の生成（上下のリング）
    for (uint32_t i = 0; i <= kRingDivide; ++i) {
        float angle = i * radianPerDivide;
        float sinTheta = std::sinf(angle);
        float cosTheta = std::cosf(angle);
        float u = float(i) / float(kRingDivide);

        // 下リングの頂点
        VertexData bottomVertex{};
        bottomVertex.position = {kRadius * cosTheta, -halfHeight, kRadius * sinTheta, 1.0f};
        bottomVertex.normal = {cosTheta, 0.0f, sinTheta};
        bottomVertex.texcoord = {u, 1.0f};
        primitiveData.vertices.push_back(bottomVertex);

        // 上リングの頂点
        VertexData topVertex{};
        topVertex.position = {kRadius * cosTheta, halfHeight, kRadius * sinTheta, 1.0f};
        topVertex.normal = {cosTheta, 0.0f, sinTheta};
        topVertex.texcoord = {u, 0.0f};
        primitiveData.vertices.push_back(topVertex);
    }

    // インデックスの生成（側面の三角形）
    for (uint32_t i = 0; i < kRingDivide; ++i) {
        uint32_t bottomLeft = i * 2;
        uint32_t topLeft = bottomLeft + 1;
        uint32_t bottomRight = bottomLeft + 2;
        uint32_t topRight = bottomLeft + 3;

        // 三角形1
        primitiveData.indices.push_back(bottomLeft);
        primitiveData.indices.push_back(bottomRight);
        primitiveData.indices.push_back(topLeft);

        // 三角形2
        primitiveData.indices.push_back(topLeft);
        primitiveData.indices.push_back(bottomRight);
        primitiveData.indices.push_back(topRight);
    }

    // その他属性
    primitiveData.color = {1.0f, 1.0f, 1.0f, 1.0f};
    primitiveData.uvMatrix = MakeIdentity4x4();

    // マップに登録
    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Cylinder, primitiveData));
}

void PrimitiveModel::CreateRing() {
    // Ringの頂点データ（垂直配置）
    PrimitiveData primitiveData{};

    const uint32_t kRingDivide = 32;
    const float kOuterRadius = 1.0f;
    const float kInnerRadius = 0.5f;
    const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

    // 頂点の生成（垂直にするためにX軸90度回転）
    for (uint32_t i = 0; i <= kRingDivide; ++i) {
        float angle = i * radianPerDivide;
        float sinTheta = std::sinf(angle);
        float cosTheta = std::cosf(angle);
        float u = float(i) / float(kRingDivide);

        // 外側の頂点（垂直配置）
        VertexData outerVertex{};
        outerVertex.position = {kOuterRadius * cosTheta, kOuterRadius * sinTheta, 0.0f, 1.0f};
        outerVertex.normal = {0.0f, 0.0f, 1.0f}; // 正面向きの法線
        outerVertex.texcoord = {u, 0.0f};
        primitiveData.vertices.push_back(outerVertex);

        // 内側の頂点（垂直配置）
        VertexData innerVertex{};
        innerVertex.position = {kInnerRadius * cosTheta, kInnerRadius * sinTheta, 0.0f, 1.0f};
        innerVertex.normal = {0.0f, 0.0f, 1.0f}; // 正面向きの法線
        innerVertex.texcoord = {u, 1.0f};
        primitiveData.vertices.push_back(innerVertex);
    }

    // インデックスの生成
    for (uint32_t i = 0; i < kRingDivide; ++i) {
        uint32_t outerCurrent = i * 2;
        uint32_t innerCurrent = outerCurrent + 1;
        uint32_t outerNext = outerCurrent + 2;
        uint32_t innerNext = outerCurrent + 3;

        // 三角形1（外側の頂点から内側へ）
        primitiveData.indices.push_back(outerCurrent);
        primitiveData.indices.push_back(innerCurrent);
        primitiveData.indices.push_back(outerNext);

        // 三角形2（内側の頂点から外側へ）
        primitiveData.indices.push_back(innerCurrent);
        primitiveData.indices.push_back(innerNext);
        primitiveData.indices.push_back(outerNext);
    }

    primitiveData.color = {1.0f, 1.0f, 1.0f, 1.0f};
    primitiveData.uvMatrix = MakeIdentity4x4();

    // マップに挿入
    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Ring, primitiveData));
}