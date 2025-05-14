#include "Object3dCommon.h"
#include "Log/Logger.h"

Object3dCommon *Object3dCommon::instance = nullptr;

Object3dCommon *Object3dCommon::GetInstance() {
    if (instance == nullptr) {
        instance = new Object3dCommon();
    }
    return instance;
}

void Object3dCommon::Finalize() {
    delete instance;
    instance = nullptr;
}

void Object3dCommon::Initialize() {
    // 引数で受け取ってメンバ変数に記録する
    dxCommon_ = DirectXCommon::GetInstance();

    psoManager_ = PipeLineManager::GetInstance();
}

void Object3dCommon::DrawCommonSetting() {
    psoManager_->DrawCommonSetting(PipelineType::kStandard);
}

void Object3dCommon::skinningDrawCommonSetting() {
    psoManager_->DrawCommonSetting(PipelineType::kSkinning);
}

void Object3dCommon::SetBlendMode(BlendMode blendMode) {
    psoManager_->DrawCommonSetting(PipelineType::kStandard, blendMode);
}
