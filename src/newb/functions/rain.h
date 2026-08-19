#ifndef RAIN_H
#define RAIN_H

#include "clouds.h"
#include "detection.h"
#include "noise.h"
#include "sky.h"
#include "water.h"

float nlWindblow(vec3 pos,float t) {
  vec2 p=pos.xy/(1.0+max(pos.z,0.0));
  float a=sin(4.0*p.x+2.0*p.y+2.0*t+3.0*p.y*p.x);
  float b=sin(p.y*2.0+0.2*t);
  float c=sin(7.0*p.x-p.y+0.7*t);
  return 0.18*(a*b+0.35*c)*(a*b+0.35*c);
}

vec4 nlRefl(inout vec4 color,nl_skycolor skycol,nl_environment env,vec3 viewDir,vec3 wPos,vec3 tiledCpos,vec3 CAMERA_POS,vec3 torchColor,vec2 lit,float camDist,float renderDist,highp float t) {
  vec4 wetRefl=vec4_splat(0.0);
  #ifndef NL_GROUND_REFL
  if(env.rainFactor>0.0) {
  #endif
    float wetness=lit.y*lit.y;
    float endDist=renderDist*0.72;
    if(camDist<endDist) {
      float cosR=max(viewDir.y,0.0);
      float puddles=max(1.0-NL_GROUND_RAIN_PUDDLES*fastRand(tiledCpos.xz),0.0);
      #ifndef NL_GROUND_REFL
        wetness*=puddles;
        float reflective=wetness*env.rainFactor*NL_GROUND_RAIN_WETNESS;
      #else
        float reflective=NL_GROUND_REFL;
        if(!env.end&&!env.nether) reflective*=wetness;
        wetness*=puddles;
        reflective=mix(reflective,wetness,env.rainFactor);
      #endif
      if(wPos.y<0.0) {
        viewDir.y=-viewDir.y;
        wetRefl.rgb=nlRenderSky(skycol,env,viewDir,t,false);
        #ifdef NL_CLOUD_AURORA_REFLECTION
          vec4 cloudRefl=nlCloudAuroraReflection(skycol,env,viewDir,wPos,CAMERA_POS,t);
          wetRefl.rgb=mix(wetRefl.rgb,cloudRefl.rgb,cloudRefl.a);
        #endif
        wetRefl.rgb+=torchColor*lit.x*NL_TORCHLIGHT_INTENSITY;
        float wave=0.5+0.5*sin(0.55*wPos.x+0.73*wPos.z+1.6*t);
        float fres=calculateFresnel(cosR,0.028+0.018*wave);
        wetRefl.a=fres*reflective;
        wetRefl.a*=clamp(2.0-2.0*camDist/endDist,0.0,1.0);
      }
    }
    color.rgb*=1.0-0.36*wetness*env.rainFactor;
  #ifndef NL_GROUND_REFL
  }
  #endif
  return wetRefl;
}

#endif
