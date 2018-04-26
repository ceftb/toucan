#version 450

layout(location = 0) out vec4 out_color;
layout(push_constant) uniform Constants {
  layout(offset = 80) vec4 color;
} constants;

void main(){

	//out_color = vec4(0.7, 0.7, 0.7, 1.0);
  out_color = constants.color;

}
