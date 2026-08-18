#ifndef WATER_H
#define WATER_H

#include "utils.h"
#include "detection.h"
#include "sky.h"
#include "clouds.h"
#include "noise.h"

float calculateFresnel(float cosR, float r0) {
  float c=saturate(abs(cosR));
  float f=1.0-c;
  float f2=f*f;
  float f5=f2*f2*f;
  return saturate(r0+(1.0-r0)*f5);
}

vec4 nlWater(
  inout vec4 color, inout vec3 wPos, nl_skycolor skycol, nl_environment env, vec4 COLOR, vec3 viewDir,
  vec3 cPos, vec3 tiledCpos, vec3 gPos, vec3 CAMERA_POS, vec3 light, vec3 torchColor, vec2 lit,
  float fractCposY, float camDist, highp float t
) {
  vec2 wavePos=gPos.xz+vec2(0.37,-0.19)*gPos.yy;
  float n0=movingNoise2D(wavePos,NL_WATER_WAVE_SPEED*t,0.72);
  float n1=movingNoise2D(wavePos*1.73+vec2(2.4,-1.1),NL_WATER_WAVE_SPEED*t*1.31,0.58);
  vec2 bump=vec2(n0-0.5,n1-0.5);

  vec3 nrm;
  if(fractCposY>0.0) {
    nrm=vec3(bump.x*NL_WATER_BUMP,-1.0,bump.y*NL_WATER_BUMP);
  } else {
    vec2 lateral=safeNormalize(vec3(viewDir.x,0.0,viewDir.z)).xz;
    vec2 sideWave=bump*(0.55+0.45*sin(2.0*t*NL_WATER_WAVE_SPEED+cPos.y*PI_HALF));
    nrm=vec3(lateral.x+sideWave.x*NL_WATER_BUMP,sideWave.y*NL_WATER_BUMP,lateral.y+sideWave.y*NL_WATER_BUMP);
  }
  nrm=safeNormalize(nrm);

  float cosR=dot(nrm,viewDir);
  vec3 reflDir=viewDir-2.0*cosR*nrm;
  reflDir=safeNormalize(reflDir);
  vec3 waterRefl=nlRenderSky(skycol,env,reflDir,t,false);

  #ifdef NL_CLOUD_AURORA_REFLECTION
    if(reflDir.y<0.15) {
      vec4 cloudRefl=nlCloudAuroraReflection(skycol,env,reflDir,wPos,CAMERA_POS,t);
      waterRefl=mix(waterRefl,cloudRefl.rgb,cloudRefl.a*0.8);
    }
  #endif

  float sparkle=0.5+0.5*sin(10.0*reflDir.x+1.7*t)*sin(12.0*reflDir.z-1.1*t);
  sparkle=smoothstep(0.72,1.0,sparkle);
  waterRefl+=torchColor*NL_TORCHLIGHT_INTENSITY*lit.x*(0.12+0.38*sparkle);

  float lightMask=0.14+0.86*saturate(lit.y);
  waterRefl*=lightMask;
  if(env.end) waterRefl*=0.72+0.28*lit.y;

  #ifdef NL_WATER_REFL_MASK
    float mask=0.5+0.5*sin(reflDir.x*10.0+reflDir.z*4.0)*sin(reflDir.z*7.0-reflDir.x*2.0);
    float reflectionMask=smoothstep(0.18,0.86,mask*0.35+0.65*abs(reflDir.y));
    waterRefl*=0.72+0.28*reflectionMask;
  #endif

  cosR=abs(cosR);
  float fresnel=calculateFresnel(cosR,0.025);
  float depthFade=saturate(0.35+0.65*(1.0-abs(fractCposY)));
  float viewDepth=pow(1.0-cosR,1.15);
  float opacity=saturate(mix(0.18,0.92,viewDepth)*NL_WATER_TRANSPARENCY+0.08*depthFade);
  float waterLum=0.22+0.78*luminance(light+vec3_splat(0.001));
  color.rgb=mix(color.rgb,NL_WATER_TINT*waterLum,0.42+0.26*(1.0-fresnel));
  color.rgb*=0.88+0.12*depthFade;
  color.a=mix(COLOR.a*NL_WATER_TRANSPARENCY,1.0,opacity*opacity);

  #ifdef NL_WATER_WAVE
    if(camDist<18.0) {
      float displacement=(bump.x*0.65+bump.y*0.35)*NL_WATER_BUMP;
      wPos.y-=0.38*displacement;
    }
  #endif

  return vec4(waterRefl,fresnel);
}

#endif
