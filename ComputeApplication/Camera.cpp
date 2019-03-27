#include "Camera.hpp"
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
  : pos(0.f, 0.f, 0.f)
  , yaw(0.f), pitch(0.f), roll(0.f)
  , lookSpeed(8.f)
{
  view = glm::mat4(1.f);
}

void Camera::SetPosition(glm::vec3 p)
{
  pos = p;
}

void Camera::SetRotation(float _pitch, float _yaw, float _roll)
{
  pitch = _pitch;
  yaw = _yaw;
  roll = _roll;
}

void Camera::Update()
{
  glm::vec4 up, position, lookAt;
  float _yaw, _pitch, _roll;
  glm::mat4 rotMat;

  up = glm::vec4(0.f, -1.f, 0.f, 1.f);
  position = glm::vec4(pos, 1.f);
  lookAt = glm::vec4(0.f, 0.f, 1.f, 1.f); // Default, look down the z-axis

  _pitch = pitch * 0.0174532925f;
  _yaw = yaw * 0.0174532925f;
  _roll = roll * 0.0174532925f;

  rotMat = glm::yawPitchRoll(_yaw, _pitch, _roll);

  // Transform lookAt & up
  lookAt = rotMat * lookAt;
  up = rotMat * up;

  lookAt = position + lookAt;

  {
    glm::vec3 eye, target, up3;
    eye = { position.x, position.y, position.z };
    target = { lookAt.x, lookAt.y, lookAt.z };
    up3 = { up.x, up.y, up.z };
    view = glm::lookAt(eye, target, up3);
  }
}

void Camera::LookAt(glm::vec3 pos, glm::vec3 target, glm::vec3 up)
{
  glm::vec3 lookAt = glm::normalize(target + pos);
  view = glm::lookAt(pos, lookAt, up);
}

void Camera::GetViewMatrix(glm::mat4 & out)
{
  out = view;
}

void Camera::GetBaseViewMatrix(glm::mat4 & out)
{
  glm::vec3 up, position, look;

  up = { 0.f, 1.f, 0.f};
  position = { 0.f, 0.f, -10.f};
  look = { 0.f, 0.f, 0.f};

  out = glm::lookAtLH(position, look, up);
}

void Camera::SetFrameTime(float dt)
{
  frameTime = dt;
}

void Camera::MoveForward()
{
  float rads;

  speed = frameTime * 20.f;
  rads = yaw * 0.0174532925f;

  pos.x += sinf(rads) * speed;
  pos.z += cosf(rads) * speed;
}

void Camera::MoveBackward()
{
  float rads;

  speed = frameTime * 20.f;
  rads = yaw * 0.0174532925f;

  pos.x -= sinf(rads) * speed;
  pos.z -= cosf(rads) * speed;
}

void Camera::MoveUpward()
{
  speed = frameTime * 20.f;

  pos.y -= speed;
}

void Camera::MoveDownward()
{
  speed = frameTime * 20.f;

  pos.y += speed;
}

void Camera::TurnLeft()
{
  speed = frameTime * 25.f;

  yaw -= speed;

  if (yaw < 0.f)
    yaw += 360.f;
}

void Camera::TurnRight()
{
  speed = frameTime * 25.f;

  yaw += speed;

  if (yaw > 360.f)
    yaw -= 360.f;
}

void Camera::TurnUp()
{
  speed = frameTime * 25.f;

  pitch += speed;

  if (pitch < -90.f)
    pitch = -90.f;
}

void Camera::TurnDown()
{
  speed = frameTime * 25.f;

  pitch -= speed;

  if (pitch > 90.f)
    pitch = 90.f;
}

void Camera::StrafeRight()
{
  float rads;

  speed = frameTime * 20.f;

  rads = yaw * 0.0174532925f;

  pos.z += sinf(rads) * speed;
  pos.x -= cosf(rads) * speed;
}

void Camera::StrafeLeft()
{
  float rads;

  speed = frameTime * 20.f;

  rads = yaw * 0.0174532925f;

  pos.z -= sinf(rads) * speed;
  pos.x += cosf(rads) * speed;
}

void Camera::Turn(int x, int y)
{
  yaw += static_cast<float>(x) / lookSpeed;
  pitch += static_cast<float>(y) / lookSpeed;
}
