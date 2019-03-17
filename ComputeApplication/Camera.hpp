#pragma once
#include <cmath>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "common.hpp"

//class Camera
//{
//public:
//  Camera()
//    : yaw(0.f)
//    , pitch(0.f)
//    , roll(0.f)
//  {}
//  ~Camera()
//  {}
//
//  void Update();
//
//  inline void SetPosition(const glm::vec3& NewPos) { position = NewPos; }
//  inline void SetUp(const glm::vec3& NewUp) { up = NewUp; }
//  inline void SetYaw(const float NewYaw) { yaw = NewYaw; }
//  inline void SetPitch(const float NewPitch) { pitch = NewPitch; }
//  inline void SetRoll(const float NewRoll) { roll = NewRoll; }
//
//  inline glm::vec3& GetPosition() { return position; }
//  inline glm::vec3& GetLookAt() { return lookat; }
//  inline glm::vec3& GetUp() { return up; }
//  inline glm::vec3& GetForward() { return forward; }
//  inline glm::vec3& GetRight() { return right; }
//
//private:
//  glm::vec3 position
//          , lookat
//          , up
//          , right
//          , forward;
//  float yaw, pitch, roll;
//};

class Camera
{
public:
  Camera();

  void SetPosition(glm::vec3 p);
  void SetRotation(float _pitch, float _yaw, float _roll);
  glm::vec3 GetPosition() { return pos; }
  glm::vec3 GetRotation() { return { yaw, pitch, roll }; }

  void Update();
  void LookAt(glm::vec3 pos, glm::vec3 target, glm::vec3 up);
  void GetViewMatrix(glm::mat4 & out);
  void GetBaseViewMatrix(glm::mat4 & out);

  void SetFrameTime(float dt);

  void MoveForward();
  void MoveBackward();
  void MoveUpward();
  void MoveDownward();
  void TurnLeft();
  void TurnRight();
  void TurnUp();
  void TurnDown();
  void StrafeRight();
  void StrafeLeft();
  void Turn(int x, int y);

private:
  glm::vec3 pos;
  float yaw, pitch, roll;
  glm::mat4 view;

  float speed, frameTime, lookSpeed;
};
