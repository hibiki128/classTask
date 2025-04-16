#pragma once
#include"ViewProjection/ViewProjection.h"
#include "DebugCamera/DebugCamera.h"
#include "Input.h"
#include "Object/Object3d.h"
#include "Object/Object3dCommon.h"
#include "ParticleCommon.h"
#include "ParticleEditor.h"
#include "ParticleEmitter.h"
#include "SpriteCommon.h"
#include "WorldTransform.h"
#include "Audio.h"
#include "LightGroup.h"
#include "line/DrawLine3D.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG
class SceneManager;
class BaseScene
{
public:

	virtual ~BaseScene() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	virtual void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void DrawForOffScreen();

	virtual void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

	virtual ViewProjection* GetViewProjection() = 0; 

protected:
	// シーンマネージャ
	SceneManager* sceneManager_ = nullptr;
};

