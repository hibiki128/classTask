#pragma once
#include "Object/Base/BaseObject.h"
class Player : public BaseObject {
  public:
    void Init(const std::string objName) override;
    void Update()override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

  private:
    void Move();

};
