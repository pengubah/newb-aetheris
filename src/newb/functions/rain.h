#ifndef RAIN_H
#define RAIN_H

#include "clouds.h"
#include "detection.h"
#include "noise.h"
#include "sky.h"
#include "water.h"

float nlWindblow(vec3 pos, float t){
  vec2 p=pos.xy/max(1.0+abs(pos.z),0.25);
  float a=sin(3.4*p.x+1.7*p.y+1.6*t+2.4*p.x*p.y);
  float b=sin(6.1*p.y-p.x+0.9*t);
  float c=sin(1.8*p.x+4.0*p.y-0.6*t);
  float val=0.48*a+0.34*b+0.18*c;
  return 0.18+0.82*(0.5+0.5*val)*(0.5+0.5*val);
}

vec4 nlRefl(
  inout vec4 color, nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 wPos, vec3 tiledCpos,
  vec3 CAMERA_POS, vec3 torchColor, vec2 lit, float camDist, float renderDist, highp float t
) {
  vec4 wetRefl=vec4_splat(0.0);
  #ifndef NL_GROUND_REFL
  if(env.rainFactor>0.0) {
  #endif
    float lightWet=pow(saturate(lit.y),1.35);
    float micro=0.5+0.5*sin(tiledCpos.x*4.7+t)*sin(tiledCpos.z*5.3-t*0.7);
    micro=mix(0.72,1.0,micro*micro);
    float puddles=1.0-NL_GROUND_RAIN_PUDDLES*fastRand(tiledCpos.xz*0.7+vec2(3.1,8.2));
    puddles=saturate(puddles);
    float wetness=lightWet*micro;

    float endDist=max(renderDist*0.58,1.0);
    if(camDist<endDist) {
      float viewUp=saturate(viewDir.y);
      float horizon=1.0-viewUp;
      float reflective;
      #ifndef NL_GROUND_REFL
        reflective=wetness*env.rainFactor*NL_GROUND_RAIN_WETNESS*puddles;
      #else
        reflective=NL_GROUND_REFL;
        if(!env.end&&!env.nether) reflective*=wetness;
        reflective=mix(reflective,wetness,env.rainFactor);
      #endif
      reflective*=smoothstep(0.0,0.9,reflective);
      reflective*=0.55+0.45*horizon;

      if(wPos.y<0.0) {
        vec3 rDir=viewDir;
        rDir.y=-rDir.y;
        rDir=safeNormalize(rDir);
        wetRefl.rgb=nlRenderSky(skycol,env,rDir,t,false);
        #ifdef NL_CLOUD_AURORA_REFLECTION
          vec4 cloudRefl=nlCloudAuroraReflection(skycol,env,rDir,wPos,CAMERA_POS,t);
          wetRefl.rgb=mix(wetRefl.rgb,cloudRefl.rgb,cloudRefl.a*0.7);
        #endif
        float torchF=0.12+0.28*smoothstep(0.35,1.0,lit.x);
        wetRefl.rgb+=torchColor*lit.x*NL_TORCHLIGHT_INTENSITY*torchF;
        float fres=calculateFresnel(saturate(viewUp),0.018);
        wetRefl.a=fres*reflective;
        wetRefl.a*=smoothstep(0.0,endDist,camDist)*0.72+0.28;
      }
    }
    color.rgb*=1.0-0.26*wetness*env.rainFactor;
  #ifndef NL_GROUND_REFL
  }
  #endif
  return wetRefl;
}

#endif
