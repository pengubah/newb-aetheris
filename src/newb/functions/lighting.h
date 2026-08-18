#ifndef LIGHTING_H
#define LIGHTING_H

#include "detection.h"
#include "sky.h"
#include "utils.h"
#include "noise.h"
#include "clouds.h"

vec3 sunLightTint(float dayFactor, float rain) {
  float df=dayFactor;
  float night=smoothstep(0.08,-0.18,df);
  float dawn=pow(saturate(1.0-df*df),2.35);
  float lowSun=smoothPulse(df,-0.42,0.48);
  float horizonWarm=smoothstep(0.02,0.92,lowSun);

  vec3 day=NL_NOON_SUNLIGHT_COL;
  vec3 moon=NL_NIGHT_MOONLIGHT_COL*(0.72+0.28*night);
  vec3 tint=mix(day,moon,night);
  vec3 dawnCol=mix(NL_DAWN_SUNLIGHT_COL,vec3(1.0,0.38,0.12),0.28*horizonWarm);
  tint=mix(tint,dawnCol,dawn*0.92);

  float rainGray=luminance(tint);
  vec3 overcast=vec3(rainGray*0.92,rainGray*0.96,rainGray*1.02);
  tint=mix(tint,overcast,saturate(rain*0.78));
  return max(tint,vec3_splat(0.0));
}

vec3 nlLighting(
  sampler2D tex, nl_skycolor skycol, nl_environment env, vec3 wPos, out vec3 torchColor, vec3 COLOR,
  vec2 uv1, vec2 lit, bool isTree, float shade, highp float t, float renderdistance, float TIME_OF_DAY, vec3 CAMERA_POS
) {
  if(env.underwater) torchColor=NL_UNDERWATER_TORCH_COL;
  else if(env.end) torchColor=NL_END_TORCH_COL;
  else if(env.nether) torchColor=NL_NETHER_TORCH_COL;
  else torchColor=NL_OVERWORLD_TORCH_COL;

  float blockLight=saturate(uv1.x);
  float skyLight=saturate(uv1.y);
  float torchCurve=pow(blockLight,0.78);
  float torchAttenuation=NL_TORCHLIGHT_INTENSITY*torchCurve/(0.64+0.30*blockLight);
  #ifdef NL_BLINKING_TORCH
    float flicker=0.94+0.06*noise1D(t*6.0+dot(wPos,vec3(0.13,0.17,0.11)));
    torchAttenuation*=flicker;
  #endif
  vec3 torchLight=torchColor*torchAttenuation;

  float gameBrightness=saturate(texelFetch(tex,ivec2(0,0),0).g);
  vec3 light=vec3_splat(0.0);
  float lum;

  if(env.nether || env.end) {
    vec3 ambient=env.end ? NL_END_AMBIENT : NL_NETHER_AMBIENT;
    float environmentStrength=env.end ? (0.78+0.22*skyLight) : (0.88+0.12*skyLight);
    light=ambient*gameBrightness*environmentStrength;
    vec3 envSky=env.end ? (0.48*skycol.horizon+0.16*skycol.zenith) : (0.65*skycol.horizon+0.10*skycol.zenith);
    light+=envSky*(0.35+0.65*skyLight);
    if(env.end) {
      float voidShade=0.72+0.28*skyLight;
      light*=voidShade;
    }
  } else {
    float dawn=pow(saturate(1.0-env.dayFactor*env.dayFactor),2.3);
    float night=smoothstep(0.06,-0.22,env.dayFactor);
    float solarHeight=saturate(env.sunDir.y*0.5+0.5);
    float horizonSun=pow(saturate(1.0-abs(env.sunDir.y)),1.35);
    float sunDistanceFade=1.0;
    float pathYaw=degToRad(NL_SUN_PATH_YAW);
    vec2 pathDir=vec2(cos(pathYaw),sin(pathYaw));
    vec2 localXZ=wPos.xz/max(length(wPos.xz),0.001);
    float facing=0.5+0.5*dot(localXZ,pathDir);
    sunDistanceFade=smoothstep(0.0,1.0,0.45+0.55*facing);
    sunDistanceFade*=1.0-smoothstep(0.70,1.0,abs(wPos.y)/max(renderdistance,16.0))*0.18;
    float sunAtten=mix(0.72,1.0,solarHeight);
    sunAtten=mix(sunAtten,sunAtten*sunDistanceFade,dawn*0.75);
    sunAtten*=1.0-0.48*env.rainFactor;
    sunAtten*=0.92+0.08*horizonSun;

    float shadow=1.0-NL_SHADOW_INTENSITY*(1.0-skyLight);
    shadow=max(shadow,0.12+0.88*lit.y);
    shadow*=mix(0.78,1.0,saturate(shade));
    shadow*=1.0-0.18*env.rainFactor;

    #if defined(NL_CLOUD_SHADOW) && (NL_CLOUD_TYPE == 1 || NL_CLOUD_TYPE == 2)
      vec3 mainLightDir=env.sunDir.y>0.0?env.sunDir:env.moonDir;
      vec3 gPos=wPos+CAMERA_POS;
      float relativeHeight=gPos.y-187.0;
      float dirY=max(abs(mainLightDir.y),0.12);
      vec2 projectionOffset=relativeHeight*mainLightDir.xz/dirY;
      vec2 projectedPos=gPos.xz+projectionOffset;
      float cloudFade=smoothstep(1.0,0.25,length(0.0018*(wPos.xz+projectionOffset)));
      cloudFade*=smoothstep(-20.0,8.0,relativeHeight);
      float cmask=0.0;
      #if NL_CLOUD_TYPE == 1
        cmask=cloudNoise2D(projectedPos*NL_CLOUD1_SCALE,t,env.rainFactor);
      #elif NL_CLOUD_TYPE == 2
        projectedPos=NL_CLOUD2_SCALE*(projectedPos+vec2(1.0,0.5)*(t*NL_CLOUD2_VELOCITY));
        cmask=cloudDf(vec3(projectedPos.x,0.5,projectedPos.y),env.rainFactor,NL_CLOUD2_SHAPE);
      #endif
      cmask*=cloudFade*(1.0-dawn*0.8);
      shadow*=mix(1.0,0.36,smoothstep(0.18,0.78,cmask));
    #endif

    vec3 direct=(NL_SUNLIGHT_INTENSITY*sunAtten*shadow)*sunLightTint(env.dayFactor,env.rainFactor);
    light=direct;

    vec3 skyAmbient=0.58*skycol.horizon+0.42*skycol.zenith;
    float ambientWeight=0.18+0.82*skyLight;
    ambientWeight*=1.0-0.18*night;
    light+=skyAmbient*(ambientWeight/(1.0+luminance(direct)));

    float groundBounce=(1.0-skyLight)*0.10+0.04;
    light+=skycol.horizon*groundBounce;
  }

  lum=luminance(light);
  light+=torchLight/(0.72+lum);

  if(!(env.nether||env.end)) {
    lum=luminance(light);
    float darkBoost=gameBrightness*NL_MIN_LIGHTING_BOOST/(0.9+lum);
    darkBoost*=mix(1.0,0.74,env.rainFactor);
    light+=vec3_splat(darkBoost);
  }

  float materialMask=saturate((COLOR.g-0.08)*1.6);
  light*=mix(0.78,1.0,materialMask);
  if(isTree) {
    float leafLift=0.98+0.16*skyLight+0.08*(1.0-env.rainFactor);
    light*=leafLift;
  }
  return max(light,vec3_splat(0.0));
}

void nlUnderwaterLighting(inout vec3 light, inout vec3 pos, vec2 lit, vec2 uv1, vec3 tiledCpos, vec3 cPos, highp float t, vec3 horizonCol) {
  float depth=saturate(1.0-uv1.y);
  if(uv1.y<0.96) {
    float c0=disp(tiledCpos,NL_WATER_WAVE_SPEED*t);
    float c1=disp(tiledCpos*1.7+vec3(2.0,2.0,2.0),NL_WATER_WAVE_SPEED*t*1.27);
    float caustics=saturate(0.62*c0+0.38*c1);
    caustics=caustics*caustics*(3.0-2.0*caustics);
    float causticLight=NL_CAUSTIC_INTENSITY*caustics*(0.12+lit.y+0.62*lit.x)*(0.55+0.45*depth);
    light+=vec3_splat(NL_UNDERWATER_BRIGHTNESS*(0.45+0.55*lit.y)+causticLight);
  }
  vec3 waterTint=safeNormalize(horizonCol+vec3(0.02,0.04,0.02));
  light*=mix(vec3_splat(0.72),waterTint,0.72*depth);
  #ifdef NL_UNDERWATER_WAVE
    float waveFade=min(0.05*max(pos.z,0.0),0.6);
    float wave=sin(t*1.2+dot(cPos,vec3_splat(PI_HALF)))+0.35*sin(t*0.7+cPos.x*1.8);
    pos.xy+=NL_UNDERWATER_WAVE*waveFade*vec2(wave,0.7*wave);
  #endif
}

vec3 nlEntityLighting(nl_skycolor skycol, nl_environment env, vec3 pos, vec4 normal, vec3 wPos, mat4 world, vec4 tileLightCol, vec4 overlayCol, vec3 horizonEdgeCol, float t, float TIME_OF_DAY, float renderdistance, vec3 CAMERA_POS) {
  float skyLight=saturate(tileLightCol.b);
  float blockLight=saturate(tileLightCol.r);
  float tl=pow(max(blockLight-0.05,0.0),0.88);
  float lum;
  vec3 light;

  if(env.nether||env.end) {
    vec3 ambient=env.end?NL_END_AMBIENT:NL_NETHER_AMBIENT;
    light=ambient*(0.25+0.75*skyLight);
    light+=skycol.horizon*(0.28+0.42*skyLight);
    if(env.end) light*=0.82+0.18*skyLight;
  } else {
    float dawn=pow(saturate(1.0-env.dayFactor*env.dayFactor),2.3);
    float solar=saturate(0.5+0.5*env.sunDir.y);
    float sunAtten=mix(0.76,1.0,solar);
    sunAtten=mix(sunAtten,sunAtten*(0.74+0.26*solar),dawn);
    sunAtten*=1.0-0.45*env.rainFactor;
    vec3 direct=NL_SUNLIGHT_INTENSITY*skyLight*sunAtten*sunLightTint(env.dayFactor,env.rainFactor);
    vec3 N=safeNormalize(mul(world,normal).xyz);
    float ndl=saturate(0.30+0.70*N.y);
    light=direct*ndl;
    lum=luminance(light);
    light+=(0.62*skycol.horizon+0.38*skycol.zenith)*(0.22+0.78*skyLight)/(0.9+lum);
  }

  vec3 torchColor;
  if(env.underwater) torchColor=NL_UNDERWATER_TORCH_COL;
  else if(env.end) torchColor=NL_END_TORCH_COL;
  else if(env.nether) torchColor=NL_NETHER_TORCH_COL;
  else torchColor=NL_OVERWORLD_TORCH_COL;

  lum=luminance(light);
  float torchMask=smoothstep(0.02,0.55,blockLight-skyLight*0.35);
  light+=torchColor*(NL_TORCHLIGHT_INTENSITY*tl*torchMask)/(0.75+lum);

  if(!(env.nether||env.end)) {
    lum=luminance(light);
    light+=vec3_splat(min(blockLight,0.16)*NL_MIN_LIGHTING_BOOST/(0.95+lum));
  }

  if(env.underwater) {
    vec3 gPos=wPos+CAMERA_POS;
    float c0=0.5+0.5*sin(dot(gPos,vec3(1.8,2.4,2.1))+0.8*t);
    float c1=0.5+0.5*sin(dot(gPos,vec3(-1.2,3.1,1.4))-1.1*t);
    float caustic=c0*c1;
    light+=vec3_splat(NL_UNDERWATER_BRIGHTNESS*0.42+NL_CAUSTIC_INTENSITY*caustic*(0.08+tl));
    light*=mix(vec3_splat(0.62),safeNormalize(skycol.horizon),0.34*skyLight);
  }

  lum=luminance(light);
  light+=vec3_splat(overlayCol.a*(1.15/(0.9+lum)));
  light*=NL_ENTITY_BRIGHTNESS;
  return max(light,vec3_splat(0.0));
}

float nlEntityEdgeHighlight(vec4 edgemap) {
  #ifdef NL_ENTITY_EDGE_HIGHLIGHT
    vec2 len=min(abs(edgemap.xy),abs(edgemap.zw));
    len=len*len;
    len=len*len;
    float edge=saturate(len.x+len.y*(1.0-len.x));
    return NL_ENTITY_BRIGHTNESS+edge*NL_ENTITY_EDGE_HIGHLIGHT*(0.72+0.28*edge);
  #else
    return 1.0;
  #endif
}

vec4 nlEntityEdgeHighlightPreprocess(vec2 texcoord) {
  vec2 p=texcoord*vec2(128.0,256.0);
  vec4 edgeMap=fract(vec4(p,p*0.5));
  return 2.0*step(edgeMap,vec4_splat(0.5))-1.0;
}

vec4 nlLavaNoise(vec3 gPos, float t) {
  float n0=movingNoise2D(gPos.xz+gPos.yy,NL_LAVA_NOISE_SPEED*t,0.85);
  float n1=movingNoise2D(gPos.zx*1.35+vec2(2.0,2.0),NL_LAVA_NOISE_SPEED*t*0.7,0.55);
  float n=saturate(0.68*n0+0.32*n1);
  n=n*n*(3.0-2.0*n);
  vec3 dark=mix(vec3(0.24,0.035,0.005),vec3(0.72,0.18,0.015),n);
  vec3 hot=mix(dark,vec3(1.45,0.46,0.05),pow(n,2.2));
  return vec4(hot,n);
}

#endif
