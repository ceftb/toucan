#include <stdlib.h>
#include <string.h>
#include "graphics.h"

float* ident()
{
  float *m = malloc(16*sizeof(float));
  float _m[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  memcpy(m, _m, 16*sizeof(float));
  return m;
}

/*
float* ortho(float left, float right, float top, float bottom, float near, 
    float far)
{
  float *m = malloc(16*sizeof(float));
  orthom(left, right, top, bottom, near, far, m);
  return m;
}
*/

mat4_t 
orthom(float left, float right, float top, float bottom, float near, float far)
{
  float w = 2.0f/(right-left),
        h = 2.0f/(bottom-top),
        d = 1.0f/(near-far),
        p = -(right+left)/(right-left),
        q = -(bottom+top)/(bottom-top),
        r = near/(near-far);

  return mat4(
    w,    0.0f, 0.0f, 0.0f,
    0.0f, h,    0.0f, 0.0f,
    0.0f, 0.0f, d,    0.0f,
    p,    q,    r,    1.0f
  );
  //return m4_ortho(left, right, bottom, top, far, near);

  //memcpy(m, _m, 16*sizeof(float));
}

