#pragma once

#include "math_3d.h"

#define MAT4_SIZE 16*sizeof(float)
#define VEC4_SIZE 5*sizeof(float)

float* ident();

mat4_t 
orthom(float left, float right, float top, float bottom, float near, float far);
