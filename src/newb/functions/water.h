#ifndef WATER_H
#define WATER_H

#include "utils.h"
#include "detection.h"
#include "sky.h"
#include "clouds.h"
#include "noise.h"

float calculateFresnel(float cosR,float r0) {
  float a=1.0-cosR;
  float a2=a*a;
  return r0+(1.0-r0)*a2*a2*a;
}

vec3 waterNormal(vec3 gPos,float t,float scale) {
  float n0=movingNoise2D(gPos.xz+gPos.yy,NL_WATER_WAVE_SPEED*t,0.55);
  float n1=movingNoise2D(gPos.zx-gPos.yy*0.7,NL_WATER_WAVE_SPEED*t*0.63+17.0,0.72);
  vec2 bump=vec2(n0,n1)*2.0-1.0;
  return normalize(vec3(bump.x*scale,-1.0,bump.y*scale));
}

vec4 nlWater(inout vec4 color,inout vec3 wPos,nl_skycolor skycol,nl_environment env,vec4 COLOR,vec3 viewDir,vec3 cPos,vec3 tiledCpos,vec3 gPos,vec3 CAMERA_POS,vec3 light,vec3 torchColor,vec2 lit,float fractCposY,float camDist,highp float t) {
  vec3 nrm;
  if(fractCposY>0.0) {
    nrm=waterNormal(gPos,t,NL_WATER_BUMP*1.25);
  } else {
    float side=0.5+0.5*sin(3.0*t*NL_WATER_WAVE_SPEED+cPos.y*PI_HALF);
    vec3 base=normalize(vec3(viewDir.xz,0.0));
    nrm=normalize(vec3(base.x+side*0.18,-0.25+side*0.08,base.y-side*0.18));
  }

  float cosR=dot(nrm,viewDir);
  viewDir=normalize(viewDir-2.0*cosR*nrm);
  vec3 waterRefl=nlRenderSky(skycol,env,viewDir,t,false);

  #if defined(NL_CLOUD_AURORA_REFLECTION)
    if(viewDir.y<0.0) {
      vec4 cloudRefl=nlCloudAuroraReflection(skycol,env,viewDir,wPos,CAMERA_POS,t);
      waterRefl=mix(waterRefl,cloudRefl.rgb,cloudRefl.a);
    }
  #endif

  float ripple=0.5+0.5*sin(12.0*viewDir.x+1.5*t)*sin(10.0*viewDir.z-1.1*t);
  waterRefl+=torchColor*NL_TORCHLIGHT_INTENSITY*lit.x*(0.25+0.75*ripple*ripple);
  if(!env.end) waterRefl*=0.04+lit.y*1.18;

  #ifdef NL_WATER_REFL_MASK
    float mask=0.5+0.5*sin(viewDir.x*11.0+viewDir.z*4.0)*sin(viewDir.z*9.0-viewDir.x*3.0);
    waterRefl*=mix(0.72,1.0,smoothstep(-0.4,0.7,viewDir.y))*mix(0.84,1.0,mask*0.16);
  #endif

  cosR=abs(cosR);
  float fresnel=calculateFresnel(cosR,0.055);
  float grazing=1.0-cosR;
  float depthTint=1.0-0.35*fresnel;
  color.rgb*=0.20*NL_WATER_TINT*depthTint;
  color.a=mix(COLOR.a*NL_WATER_TRANSPARENCY,1.0,grazing*grazing);

  #ifdef NL_WATER_WAVE
    if(camDist<18.0) {
      float wave=0.5+0.5*sin(t*NL_WATER_WAVE_SPEED*2.0+gPos.x*0.45+gPos.z*0.37);
      wPos.y-=0.45*wave*NL_WATER_BUMP;
    }
  #endif

  return vec4(waterRefl,fresnel);
}

#endif
