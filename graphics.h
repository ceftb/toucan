#pragma once

#define MAT4_SIZE 16*sizeof(float)

float* ident();
float* ortho(float right, float left, float bottom, float top, float near, 
    float far);
void orthom(float left, float right, float top, float bottom, float near, 
    float far, float *m);
