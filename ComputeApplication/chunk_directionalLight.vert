#version 450

//layout(push_constant) uniform PushConstant {
//  mat4 m;
//  mat4 vp;
//} push;

layout(set = 0, binding = 0) uniform ViewProj {
  mat4 view;
  mat4 proj;
};

layout(set = 0, binding = 1) uniform Model {
  mat4 model;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 vertexNormal;
layout(location = 1) out vec3 fragPos;

void main()
{
  vertexNormal = mat3(transpose(inverse(model))) * normal;
  //vertexNormal = normal;
  fragPos = vec3(model * vec4(position, 1.0));

  gl_Position = proj * view * vec4(fragPos, 1);
}
