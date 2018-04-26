#version 450

layout(location = 0) in vec2 position;

layout(push_constant) uniform Constants {
  layout(offset = 0) mat4 world;
  layout(offset = 64) float z;
} constants;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
};

void main(){

  gl_PointSize = 5;
	gl_Position = vec4(position, constants.z, 1.0) * constants.world;

}

