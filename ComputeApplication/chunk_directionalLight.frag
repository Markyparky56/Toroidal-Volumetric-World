#version 450

layout(location = 0) in vec3 vertexNormal;

layout(location = 0) out vec4 fragColour;

void main()
{
  vec3 lightDir =  -normalize(vec3(1, 5, -5)); // TODO: switch to push constant
  float d = dot(vertexNormal, -lightDir);
  d = max(0.2, d);
  fragColour = vec4(1.0, 0.0, 1.0, 1.0) * d;
}
