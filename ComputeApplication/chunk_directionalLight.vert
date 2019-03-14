#version 450

layout(push_constant) uniform PushConstant {
  mat4 mvp;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 vertexNormal;

void main()
{
  vertexNormal = vec3(1.0);

  gl_Position = mvp * vec4(position, 1);
}
