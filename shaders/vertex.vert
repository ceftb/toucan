#version 450

layout(location = 0) in vec2 position;

layout(push_constant) uniform World {
  mat4 projection;
} world;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
};

void main(){

  gl_PointSize = 20.0;
	//gl_Position = vec4(position, 0.0, 1.0);
	gl_Position = world.projection * vec4(position, 0.0, 1.0);

}

