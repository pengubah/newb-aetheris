#ifndef TONEMAP_H
#define TONEMAP_H

#include "utils.h"

vec3 colorCorrection(vec3 col) {
  col=max(col,vec3_splat(0.0));
  #ifdef NL_EXPOSURE
    col*=NL_EXPOSURE;
  #endif

  // gentle pre-compression keeps bright sunlight luminous without clipping the scene
  float peak=max(max(col.r,col.g),col.b);
  float pre=1.0+0.12*peak;
  col/=pre;

  #if NL_TONEMAP_TYPE == 3
    const float whiteScale=0.063;
    col=col*(1.0+col*whiteScale)/(1.0+col);
  #elif NL_TONEMAP_TYPE == 4
    const float a=1.04;
    const float b=0.03;
    const float c=0.93;
    const float d=0.56;
    const float e=0.14;
    col*=0.92;
    col=clamp((col*(a*col+b))/(col*(c*col+d)+e),0.0,1.0);
  #elif NL_TONEMAP_TYPE == 2
    col=col/(1.0+col);
  #elif NL_TONEMAP_TYPE == 1
    col=1.0-exp(-col*0.82);
  #endif

  // lift the deepest shadows slightly, like the reference look, while retaining contrast
  float lum=luminance(col);
  float shadowLift=smoothstep(0.0,0.16,lum);
  col=mix(col,col+vec3_splat(0.012),1.0-shadowLift);

  col=pow(max(col,vec3_splat(0.0)),vec3_splat(1.0/NL_GAMMA));

  #ifdef NL_SATURATION
    float l=luminance(col);
    col=mix(vec3_splat(l),col,NL_SATURATION);
  #endif

  #ifdef NL_TINT
    vec3 tint=mix(NL_TINT_LOW,NL_TINT_HIGH,smoothstep(0.0,1.0,col));
    col*=tint;
  #endif

  // soft highlight rolloff after grading
  col=col/(1.0+0.10*max(col-0.72,vec3_splat(0.0)));
  return clamp(col,vec3_splat(0.0),vec3_splat(1.0));
}

vec3 colorCorrectionInv(vec3 col) {
  #ifdef NL_TINT
    col/=mix(NL_TINT_LOW,NL_TINT_HIGH,clamp(col,vec3_splat(0.0),vec3_splat(1.0)));
  #endif
  #ifdef NL_SATURATION
    col=mix(vec3_splat(dot(col,vec3(0.21,0.71,0.08))),col,1.0/NL_SATURATION);
  #endif
  float ws=0.7966;
  col=pow(max(col,vec3_splat(0.0)),vec3_splat(NL_GAMMA));
  col=col*(ws+col)/(ws+col*(1.0-ws));
  #ifdef NL_EXPOSURE
    col/=NL_EXPOSURE;
  #endif
  return col;
}

#endif
