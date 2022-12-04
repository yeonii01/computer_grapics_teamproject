#version 330 core

in vec3 FragPos;
in vec3 Normal;
uniform vec3 outColor;

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

out vec4 fColor;

void main ()
{
  vec3 ambientLight = vec3(0.5f, 0.5f, 0.5f);
  vec3 ambient = ambientLight * lightColor;
  vec3 NV = normalize(Normal);
  vec3 lightDir = normalize(lightPos - FragPos);
  float diffuseLight = max(dot(NV, lightDir), 0.0);
  vec3 diffuse = diffuseLight * lightColor;

  int shininess = 256;
  vec3 viewDir = normalize(cameraPos - FragPos);
  vec3 reflectDir = reflect(lightDir, NV);
  float specularLight = max (dot(viewDir, reflectDir), 0.0);
  specularLight = pow(specularLight, shininess);
  vec3 specular = specularLight * lightColor;

  vec3 result = (diffuse+specular+ambient) * outColor;

  fColor = vec4((result), 1.0);
}
