#pragma once
#include <cmath>
#include <glm/vec3.hpp>

constexpr double PI = 3.1415926535897932384626433832795;

class Camera
{
public:
  Camera()
    : yaw(0.f)
    , pitch(0.f)
    , roll(0.f)
  {}
  ~Camera()
  {}

  void Update();

  inline void SetPosition(const glm::vec3& NewPos) { position = NewPos; }
  inline void SetUp(const glm::vec3& NewUp) { up = NewUp; }
  inline void SetYaw(const float NewYaw) { yaw = NewYaw; }
  inline void SetPitch(const float NewPitch) { pitch = NewPitch; }
  inline void SetRoll(const float NewRoll) { roll = NewRoll; }

  inline glm::vec3& GetPosition() { return position; }
  inline glm::vec3& GetLookAt() { return lookat; }
  inline glm::vec3& GetUp() { return up; }
  inline glm::vec3& GetForward() { return forward; }
  inline glm::vec3& GetRight() { return right; }

private:
  glm::vec3 position
          , lookat
          , up
          , right
          , forward;
  float yaw, pitch, roll;
};
