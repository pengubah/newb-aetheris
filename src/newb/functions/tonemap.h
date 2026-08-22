#ifndef TONEMAP_H
#define TONEMAP_H

#include "utils.h"

vec3 colorCorrection(vec3 col) {
  #ifdef NL_EXPOSURE
    col *= NL_EXPOSURE;
  #endif

  float preLum = luminance(col);
  float preShoulder = smoothstep(1.5,7.0,preLum);
  col = mix(col,vec3_splat(preLum),0.06*preShoulder);

  #if NL_TONEMAP_TYPE == 3
    const float whiteScale = 0.063;
    col = col*(1.0+col*whiteScale)/(1.0+col);
  #elif NL_TONEMAP_TYPE == 4
    const float a = 1.04;
    const float b = 0.03;
    const float c = 0.93;
    const float d = 0.56;
    const float e = 0.14;
    col *= 0.85;
    col = clamp((col*(a*col+b))/(col*(c*col+d)+e),0.0,1.0);
  #elif NL_TONEMAP_TYPE == 2
    col = col/(1.0+col);
  #elif NL_TONEMAP_TYPE == 1
    col = 1.0-exp(-col*0.8);
  #endif

  float lum = max(luminance(col),0.0001);
  float highlight = smoothstep(0.68,0.99,lum);
  vec3 neutral = vec3_splat(lum);
  col = mix(col,neutral,0.045*highlight);

  col = pow(max(col,vec3_splat(0.0)),vec3_splat(1.0/NL_GAMMA));

  #ifdef NL_SATURATION
    float finalLum = luminance(col);
    col = mix(vec3_splat(finalLum),col,NL_SATURATION);
  #endif

  #ifdef NL_TINT
    col *= mix(NL_TINT_LOW,NL_TINT_HIGH,col);
  #endif

  return max(col,vec3_splat(0.0));
}

vec3 colorCorrectionInv(vec3 col) {
  #ifdef NL_TINT
    col /= mix(NL_TINT_LOW,NL_TINT_HIGH,col);
  #endif

  #ifdef NL_SATURATION
    col = mix(vec3_splat(dot(col,vec3(0.21,0.71,0.08))),col,1.0/NL_SATURATION);
  #endif

  float ws = 0.7966;
  col = pow(max(col,vec3_splat(0.0)),vec3_splat(NL_GAMMA));
  col = col*(ws+col)/(ws+col*(1.0-ws));

  #ifdef NL_EXPOSURE
    col /= NL_EXPOSURE;
  #endif

  return max(col,vec3_splat(0.0));
}

#endif
