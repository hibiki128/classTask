#pragma once
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include "json.hpp"
#include <fstream>

class HitStop {
  public:
    void Initialize();
    void Update();
    void Start();
    void imgui();
    bool IsActive() const { return isActive_; }

  private:
    void LoadSettings();
    void SaveSettings();

  private:
    float stopDuration_ = 0.1f; // 停止時間（秒）
    float elapsedTime_ = 0.0f;  // 経過時間
    bool isActive_ = false;
};
