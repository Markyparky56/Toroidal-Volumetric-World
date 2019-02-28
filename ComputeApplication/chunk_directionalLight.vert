#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(set = 0, binding = 0) uniform UniformBuffer {
  mat4 modelview;
  mat4 projection;
};

layout(location = 0) out vec3 vertexNormal;

void main()
{
  vertexNormal = normal;

  gl_Position = projection * modelview * vec4(position, 1);
}