#version 450 core

layout(location = 0) in vec2 position;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
};

void main(){

  gl_PointSize = 20.0;
	gl_Position = vec4(position, 0.0, 1.0);

}

