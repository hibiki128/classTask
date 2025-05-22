#include "Object3d.h"
#include "Object3dCommon.h"
#include "cassert"
#include "myMath.h"
#include <Model/ModelManager.h>
#include <Texture/TextureManager.h>
#include <line/DrawLine3D.h>

void Object3d::Initialize() {
    objectCommon_ = std::make_unique<Object3dCommon>();
    objectCommon_->Initialize();

    dxCommon_ = DirectXCommon::GetInstance();

    lightGroup = LightGroup::GetInstance();

    CreateTransformationMatrix();
}

void Object3d::CreateModel(const std::string &filePath) {
    filePath_ = filePath;

    ModelManager::GetInstance()->LoadModel(filePath_);

    // モデルを検索してセットする
    model = ModelManager::GetInstance()->FindModel(filePath_);

    // マルチマテリアル対応の初期化
    InitializeMaterials();

    if (model->IsGltf()) {
        currentModelAnimation_ = std::make_unique<ModelAnimation>();
        currentModelAnimation_->SetModelData(model->GetModelData());
        currentModelAnimation_->Initialize("resources/models/", filePath_);

        model->SetAnimator(currentModelAnimation_->GetAnimator());
        model->SetBone(currentModelAnimation_->GetBone());
        model->SetSkin(currentModelAnimation_->GetSkin());
    }
}

void Object3d::CreatePrimitiveModel(const PrimitiveType &type) {
    model = ModelManager::GetInstance()->FindModel(ModelManager::GetInstance()->CreatePrimitiveModel(type));

    InitializeMaterials();
}

void Object3d::Update(const WorldTransform &worldTransform, const ViewProjection &viewProjection) {
    if (lightGroup) {
        lightGroup->Update(viewProjection);
    }
    Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

    if (worldTransform.parent_) {
        worldMatrix *= worldTransform.parent_->matWorld_;
    }
    Matrix4x4 worldViewProjectionMatrix;
    const Matrix4x4 &viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;
    worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

    transformationMatrixData->WVP = worldViewProjectionMatrix;
    transformationMatrixData->World = worldTransform.matWorld_;
    Matrix4x4 worldInverseMatrix = Inverse(worldMatrix);
    transformationMatrixData->WorldInverseTranspose = Transpose(worldInverseMatrix);
}

void Object3d::AnimationUpdate(bool roop) {
    if (currentModelAnimation_) {
        currentModelAnimation_->Update(roop);
    }
}

void Object3d::SetAnimation(const std::string &fileName) {
    // すでにセット済みのアニメーションなら何もしない
    if (fileName == filePath_) {
        return;
    }

    // modelAnimations_ 内に fileName に対応するアニメーションがあるか検索
    auto it = modelAnimations_.find(fileName);

    // アニメーションが見つからなかった場合、強制的にプログラムを停止
    assert(it != modelAnimations_.end() && "Error: Animation file not found in modelAnimations_!");

    // 見つかったアニメーションを shared_ptr に格納
    currentModelAnimation_ = it->second;

    // Animator などを model にセット
    model->SetAnimator(currentModelAnimation_->GetAnimator());
    model->SetBone(currentModelAnimation_->GetBone());
    model->SetSkin(currentModelAnimation_->GetSkin());
    currentModelAnimation_->GetAnimator()->SetIsAnimation(true);
    currentModelAnimation_->GetAnimator()->SetAnimationTime(0.0f);

    // ファイルパスを更新
    filePath_ = fileName;
}

void Object3d::AddAnimation(const std::string &fileName) {
    auto animation = std::make_unique<ModelAnimation>();

    animation->SetModelData(model->GetModelData());
    animation->Initialize("resources/models/", fileName);
    animation->GetAnimator()->SetAnimationTime(0.0f);

    modelAnimations_.emplace(fileName, std::move(animation));
}

void Object3d::Draw(const WorldTransform &worldTransform, const ViewProjection &viewProjection, ObjColor *color, bool Lighting) {
   objectCommon_->SetBlendMode(blendMode_);

   Update(worldTransform, viewProjection);

   if (model->IsGltf()) {
       if (currentModelAnimation_->GetAnimator()->HaveAnimation()) {
           HaveAnimation = true;
           objectCommon_->skinningDrawCommonSetting();
       } else {
           HaveAnimation = false;
       }
   }

   // wvp用のCBufferの場所を設定
   dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

   // ライティング設定
   if (lightGroup) {
       lightGroup->Draw();
   }

   if (model) {
       if (color) {
           Vector4 overrideColor = color->GetColor();
           for (size_t i = 0; i < model->GetMaterialCount(); ++i) {
               model->SetMaterialColor(static_cast<uint32_t>(i), overrideColor);
           }
       }
       std::vector<Material> materialRefs;
       for (const auto& material : materials_) {
           materialRefs.push_back(*material);
       }

       model->Draw(objectCommon_.get(), materialRefs);
   }
}

void Object3d::DrawSkeleton(const WorldTransform &worldTransform, const ViewProjection &viewProjection) {
    Update(worldTransform, viewProjection);
    // スケルトンデータを取得
    const Skeleton &skeleton = currentModelAnimation_->GetSkeletonData();

    // 各ジョイントを巡回して親子関係の線を生成
    for (const auto &joint : skeleton.joints) {
        // 親がいない場合、このジョイントはルートなのでスキップ
        if (!joint.parent.has_value()) {
            continue;
        }

        // 親ジョイントを取得
        const auto &parentJoint = skeleton.joints[*joint.parent];

        // 親と子のスケルトン空間座標を取得
        Vector3 parentPosition = ExtractTranslation(parentJoint.skeletonSpaceMatrix);
        Vector3 childPosition = ExtractTranslation(joint.skeletonSpaceMatrix);

        // 線の色を設定（デフォルトで白色）
        Vector4 lineColor = {1.0f, 1.0f, 1.0f, 1.0f};

        // LineManagerに現在の線分を登録
        DrawLine3D::GetInstance()->SetPoints(parentPosition, childPosition, lineColor);
    }
}

void Object3d::SetModel(const std::string &filePath) {
    // モデルを検索してセットする
    ModelManager::GetInstance()->LoadModel(filePath);
    model = ModelManager::GetInstance()->FindModel(filePath);

    // マルチマテリアル対応の初期化
    InitializeMaterials();

    if (model->IsGltf()) {
        currentModelAnimation_->SetModelData(model->GetModelData());
        currentModelAnimation_->Initialize("resources/models/", filePath);

        model->SetAnimator(currentModelAnimation_->GetAnimator());
        model->SetBone(currentModelAnimation_->GetBone());
        model->SetSkin(currentModelAnimation_->GetSkin());
    }
}

void Object3d::SetTexture(const std::string &filePath, uint32_t materialIndex) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    materials_[materialIndex]->GetMaterialDataGPU()->textureFilePath = filePath;
    TextureManager::GetInstance()->LoadTexture(filePath);
    materials_[materialIndex]->GetMaterialDataGPU()->textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);

    // モデルにも反映
    if (model) {
        model->GetMaterialData()[materialIndex] = *materials_[materialIndex]->GetMaterialDataGPU();
    }
}

void Object3d::SetAllTexturesIndex(const std::string &filePath) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetTexture(filePath, static_cast<uint32_t>(i));
    }
}

void Object3d::SetMaterialColor(uint32_t materialIndex, const Vector4 &color) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    materials_[materialIndex]->GetMaterialDataGPU()->color = color;

    // モデルにも反映
    if (model) {
        model->GetMaterialData()[materialIndex].color = color;
    }
}

void Object3d::SetAllMaterialsColor(const Vector4 &color) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetMaterialColor(static_cast<uint32_t>(i), color);
    }
}

void Object3d::SetMaterialUVTransform(uint32_t materialIndex, const Matrix4x4 &uvTransform) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    materials_[materialIndex]->GetMaterialDataGPU()->uvTransform = uvTransform;

    // モデルにも反映
    if (model) {
        model->GetMaterialData()[materialIndex].uvTransform = uvTransform;
    }
}

void Object3d::SetAllMaterialsUVTransform(const Matrix4x4 &uvTransform) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetMaterialUVTransform(static_cast<uint32_t>(i), uvTransform);
    }
}

void Object3d::SetMaterialShininess(uint32_t materialIndex, float shininess) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    materials_[materialIndex]->GetMaterialDataGPU()->shininess = shininess;

    // モデルにも反映
    if (model) {
        model->GetMaterialData()[materialIndex].shininess = shininess;
    }
}

void Object3d::SetAllMaterialsShininess(float shininess) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetMaterialShininess(static_cast<uint32_t>(i), shininess);
    }
}

void Object3d::SetShininess(float shininess) {
    //// 後方互換性：単一マテリアルがある場合はそれに、なければ全てのマテリアルに適用
    //if (material_) {
    //    material_->GetMaterialDataGPU()->shininess = shininess;
    //} else {
    //    SetAllMaterialsShininess(shininess);
    //}
}

void Object3d::CreateTransformationMatrix() {
    transformationMatrixResource = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    // 書き込むかめのアドレスを取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));
    // 単位行列を書き込んでおく
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->World = MakeIdentity4x4();
    transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();
}

void Object3d::InitializeMaterials() {
    if (!model) {
        return;
    }

    // モデルのマテリアル数に合わせてマテリアル配列を初期化
    size_t materialCount = model->GetMaterialCount();
    materials_.resize(materialCount);

    for (size_t i = 0; i < materialCount; ++i) {
        materials_[i] = std::make_unique<Material>();
        materials_[i]->Initialize();

        // モデルのマテリアルデータを取得してセット
        if (i < model->GetMaterialData().size()) {
            materials_[i]->GetMaterialData() = model->GetMaterialData()[i];
            materials_[i]->LoadTexture();
            materials_[i]->SetMaterialDataGPU(&model->GetMaterialData()[i]);
        }
    }
}

bool Object3d::IsValidMaterialIndex(uint32_t materialIndex) const {
    return materialIndex < materials_.size();
}