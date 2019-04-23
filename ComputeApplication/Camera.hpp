#pragma once
#include <cmath>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "common.hpp"

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
