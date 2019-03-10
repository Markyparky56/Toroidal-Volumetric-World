#version 450

layout(set = 0, binding = 0) uniform UniformBuffer {
  mat4 model;
  mat4 view;
  mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 vertexNormal;

void main()
{
  vertexNormal = vec3(1.0);

  gl_Position = projection * view * model * vec4(position, 1);
}
