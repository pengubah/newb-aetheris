#ifndef TONEMAP_H
#define TONEMAP_H

#include "utils.h"

vec3 colorCorrection(vec3 col) {
  col=max(col,vec3_splat(0.0));
  #ifdef NL_EXPOSURE
    col*=max(NL_EXPOSURE,0.01);
  #endif

  #if NL_TONEMAP_TYPE == 4
    // filmic ACES-style curve with gentle pre-exposure compression
    const float a=2.51;
    const float b=0.03;
    const float c=2.43;
    const float d=0.59;
    const float e=0.14;
    col*=0.92;
    col=(col*(a*col+b))/(col*(c*col+d)+e);
    col=clamp(col,vec3_splat(0.0),vec3_splat(1.0));
  #elif NL_TONEMAP_TYPE == 3
    const float whiteScale=0.063;
    col=col*(1.0+col*whiteScale)/(1.0+col);
  #elif NL_TONEMAP_TYPE == 2
    col=col/(1.0+col);
  #elif NL_TONEMAP_TYPE == 1
    col=1.0-exp(-col*0.86);
  #endif

  float lum=max(luminance(col),0.0001);
  float highlight=smoothstep(0.55,1.0,lum);
  col*=1.0-0.055*highlight;

  col=pow(max(col,vec3_splat(0.0)),vec3_splat(1.0/max(NL_GAMMA,0.05)));

  #ifdef NL_SATURATION
    float sat=clamp(NL_SATURATION,0.0,2.0);
    vec3 gray=vec3_splat(luminance(col));
    col=mix(gray,col,sat);
  #endif

  #ifdef NL_TINT
    float tintMask=smoothstep(0.06,0.82,luminance(col));
    vec3 tint=mix(NL_TINT_LOW,NL_TINT_HIGH,tintMask);
    col*=mix(vec3_splat(1.0),tint,0.35+0.35*tintMask);
  #endif

  return clamp(col,vec3_splat(0.0),vec3_splat(1.0));
}

vec3 colorCorrectionInv(vec3 col) {
  col=max(col,vec3_splat(0.0));
  #ifdef NL_TINT
    float tintMask=smoothstep(0.06,0.82,luminance(col));
    vec3 tint=mix(NL_TINT_LOW,NL_TINT_HIGH,tintMask);
    col/=max(mix(vec3_splat(1.0),tint,0.35+0.35*tintMask),vec3_splat(0.001));
  #endif

  #ifdef NL_SATURATION
    float sat=max(NL_SATURATION,0.01);
    col=mix(vec3_splat(luminance(col)),col,1.0/sat);
  #endif

  col=pow(max(col,vec3_splat(0.0)),vec3_splat(max(NL_GAMMA,0.05)));

  #if NL_TONEMAP_TYPE == 4
    // numerical inverse approximation for the ACES-style curve
    col=col/(1.0-col*0.35);
  #elif NL_TONEMAP_TYPE == 3
    float ws=0.7966;
    col=col*(ws+col)/(ws+col*(1.0-ws));
  #elif NL_TONEMAP_TYPE == 2
    col=col/max(1.0-col,vec3_splat(0.001));
  #elif NL_TONEMAP_TYPE == 1
    col=-log(max(vec3_splat(0.001),1.0-col))/0.86;
  #endif

  #ifdef NL_EXPOSURE
    col/=max(NL_EXPOSURE,0.01);
  #endif
  return max(col,vec3_splat(0.0));
}

#endif
