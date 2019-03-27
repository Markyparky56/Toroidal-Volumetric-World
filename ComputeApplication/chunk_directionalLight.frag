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

void main()
{
  float fog = clamp(length((viewPos.xyz - fragPos)*0.0033*1.0), 0.0, 1.0);
  fog = pow(fog, 1.1);

  // ambient
  vec3 ambient = lightAmbientColour * objectColour;

  // diffuse
  vec3 norm = normalize(vertexNormal);
  vec3 lightDir = normalize(lightDir);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = lightDiffuseColour * diff * objectColour;

  vec3 col = ambient + diffuse;

  if(diff > 0.0)
  {
    vec3 view = normalize(viewPos - fragPos);
    vec3 halfVec = normalize(view + lightDir);

    float shinniness = 120.0;
    vec3 spec = pow(dot(halfVec, norm), shinniness) * objectColour;

    col += spec;
  }

  fragColour = vec4(mix(col, vec3(0.5), fog), 1.0);
  //vec3 result = ambient + diffuse/* + specular*/;
  //fragColour = vec4(result, 1.0);
}
