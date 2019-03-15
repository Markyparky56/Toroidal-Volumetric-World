#version 450

layout(location = 0) in vec3 vertexNormal;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 2) uniform lightData {
  vec3 lightDir;
  vec3 viewPos;
  vec3 lightAmbientColour;
  vec3 lightDiffuseColour;
  vec3 lightSpecularColour;
  vec3 objectColour;
};

// Praise be to opengltutorial.com for their lighting tutorials
void main()
{
  // ambient
  float ambientStrength = 0.1;
  vec3 ambient = lightAmbientColour;

  // diffuse
  vec3 norm = normalize(vertexNormal);
  vec3 lightDir = normalize(-lightDir);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = lightDiffuseColour * diff * objectColour;

  // specular
  vec3 viewDir = normalize(viewPos - fragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 0.2); // Hard code shininess until you implement tri-planar texturing
  vec3 specular = lightSpecularColour * spec * objectColour;
  
  vec3 result = ambient + diffuse + specular;
  fragColour = vec4(result, 1.0);
}
