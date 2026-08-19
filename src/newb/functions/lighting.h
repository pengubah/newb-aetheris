#ifndef LIGHTING_H
#define LIGHTING_H

#include "detection.h"
#include "sky.h"
#include "utils.h"
#include "noise.h"
#include "clouds.h"

vec3 sunLightTint(float dayFactor,float rain) {
  float nightFactor=step(dayFactor,0.0);
  float dawnFactor=pow(max(1.0-dayFactor*dayFactor,0.0),3.0);
  dawnFactor*=mix(1.0,dawnFactor*dawnFactor,nightFactor);
  vec3 noon=NL_NOON_SUNLIGHT_COL;
  vec3 night=NL_NIGHT_MOONLIGHT_COL;
  vec3 dawn=NL_DAWN_SUNLIGHT_COL;
  vec3 tint=mix(noon,night,nightFactor);
  tint=mix(tint,dawn,dawnFactor);
  float desat=rain*0.72;
  tint=mix(tint,vec3_splat(luminance(tint)),desat);
  return tint;
}

float nlSunSoftness(float dayFactor,float rain) {
  float h=smoothstep(-0.20,0.35,dayFactor);
  float dawn=pow(max(1.0-dayFactor*dayFactor,0.0),2.0);
  return mix(0.70,1.0,h)*(1.0-0.25*rain)+0.25*dawn;
}

vec3 nlLighting(sampler2D tex,nl_skycolor skycol,nl_environment env,vec3 wPos,out vec3 torchColor,vec3 COLOR,vec2 uv1,vec2 lit,bool isTree,float shade,highp float t,float renderdistance,float TIME_OF_DAY,vec3 CAMERA_POS) {
  vec3 light;
  if(env.underwater) torchColor=NL_UNDERWATER_TORCH_COL;
  else if(env.end) torchColor=NL_END_TORCH_COL;
  else if(env.nether) torchColor=NL_NETHER_TORCH_COL;
  else torchColor=NL_OVERWORLD_TORCH_COL;

  float torchAttenuation=(NL_TORCHLIGHT_INTENSITY*uv1.x)/(0.5-0.45*lit.x);
  #ifdef NL_BLINKING_TORCH
    float flicker=0.5+0.5*noise1D(t*8.0)+0.20*noise1D(t*17.0);
    torchAttenuation*=0.86+0.20*flicker;
  #endif
  vec3 torchLight=torchColor*torchAttenuation;
  float gameBrightness=texelFetch(tex,ivec2(0,0),0).g;
  float lum=0.0;

  if(env.nether||env.end) {
    light=env.end?NL_END_AMBIENT:NL_NETHER_AMBIENT;
    light*=gameBrightness;
    if(env.end) {
      vec3 endSky=skycol.horizon+skycol.zenith;
      light+=endSky*0.55;
      light*=mix(NL_END_AMBIENT,skycol.horizon,0.35);
    } else {
      light*=0.5+0.5*normalize(max(NL_NETHER_AMBIENT,vec3_splat(0.001)));
      light+=skycol.horizon*0.75;
    }
    lum=luminance(light);
    light+=skycol.horizon/(1.0+lum);
  } else {
    float nightFactor=step(env.dayFactor,0.0);
    float dawnFactor=pow(max(1.0-env.dayFactor*env.dayFactor,0.0),3.0);
    dawnFactor*=mix(1.0,dawnFactor*dawnFactor,nightFactor);
    float nightIntensity=pow(max(0.5-0.5*env.dayFactor,0.0),2.0);

    float sunLightAttenuation=clamp(0.5*(((2.0*step(TIME_OF_DAY,0.5)-1.0)*(wPos.x*cos(NL_SUN_PATH_YAW)+wPos.y*sin(NL_SUN_PATH_YAW))/renderdistance)+1.0),0.0,1.0);
    sunLightAttenuation=mix(1.0,sunLightAttenuation*sunLightAttenuation,dawnFactor*0.85);
    sunLightAttenuation*=1.0-0.46*env.rainFactor;

    float shadow=step(0.93,uv1.y);
    shadow=max(shadow,(1.0-NL_SHADOW_INTENSITY+(0.68*NL_SHADOW_INTENSITY*nightIntensity))*lit.y);
    shadow*=shade>0.8?1.0:0.82;

    #if defined(NL_CLOUD_SHADOW) && (NL_CLOUD_TYPE == 1 || NL_CLOUD_TYPE == 2)
      vec3 mainLightDir=env.sunDir.y>0.0?env.sunDir:env.moonDir;
      vec3 gPos=wPos+CAMERA_POS;
      float cloudRelativeHeight=gPos.y-187.0;
      vec2 projectionOffset=cloudRelativeHeight*mainLightDir.xz/max(abs(mainLightDir.y),0.08);
      vec2 projectedPos=gPos.xz+projectionOffset;
      float cloudFade=smoothstep(1.0,0.35,length(0.0015*(wPos.xz+projectionOffset)));
      cloudFade*=clamp(-0.12*(cloudRelativeHeight-7.0),0.0,1.0);
      float cmask=0.0;
      #if NL_CLOUD_TYPE == 1
        cmask=cloudNoise2D(projectedPos*NL_CLOUD1_SCALE,t,env.rainFactor)*cloudFade;
      #elif NL_CLOUD_TYPE == 2
        projectedPos=NL_CLOUD2_SCALE*(projectedPos+vec2(1.0,0.5)*(t*NL_CLOUD2_VELOCITY));
        cmask=cloudDf(vec3(projectedPos.x,0.5,projectedPos.y),env.rainFactor,NL_CLOUD2_SHAPE)*cloudFade;
      #endif
      shadow*=0.24+0.76*smoothstep(0.70,0.0,cmask);
    #endif

    vec3 direct=(NL_SUNLIGHT_INTENSITY*shadow*sunLightAttenuation)*sunLightTint(env.dayFactor,env.rainFactor);
    direct*=nlSunSoftness(env.dayFactor,env.rainFactor);
    light=direct;

    lum=luminance(light);
    float ambientStrength=uv1.y*(0.70+0.30*(1.0-env.rainFactor));
    vec3 skyAmbient=(skycol.horizon+skycol.zenith)*ambientStrength/(1.0+lum);
    skyAmbient*=1.0+0.25*nightIntensity;
    light+=skyAmbient;
  }

  lum=luminance(light);
  light+=torchLight/(1.0+lum);

  if(!(env.nether||env.end)) {
    lum=luminance(light);
    float boost=gameBrightness*(NL_MIN_LIGHTING_BOOST/(1.0+lum));
    light+=vec3_splat(boost);
  }

  float materialLift=mix(0.78,1.0,clamp(COLOR.g,0.0,1.0));
  light*=materialLift;
  if(isTree) light*=1.16+0.10*uv1.y;
  light*=0.94+0.06*nlContrast(lit.y,1.5);
  return light;
}

void nlUnderwaterLighting(inout vec3 light,inout vec3 pos,vec2 lit,vec2 uv1,vec3 tiledCpos,vec3 cPos,highp float t,vec3 horizonCol) {
  if(uv1.y<0.9) {
    float caustics=disp(tiledCpos,NL_WATER_WAVE_SPEED*t);
    caustics*=caustics;
    float ripple=0.5+0.5*sin(dot(tiledCpos,vec3(1.7,2.3,2.1))+1.2*t);
    light+=vec3_splat(NL_UNDERWATER_BRIGHTNESS)+NL_CAUSTIC_INTENSITY*(0.55*caustics+0.45*ripple)*(0.15+lit.y+lit.x*0.7);
  }
  vec3 waterCol=max(normalize(max(horizonCol,vec3_splat(0.001))),vec3_splat(0.001));
  light*=mix(waterCol,normalize(max(NL_UNDERWATER_TINT,vec3_splat(0.001))),lit.y*0.62);
  #ifdef NL_UNDERWATER_WAVE
    pos.xy+=NL_UNDERWATER_WAVE*min(0.05*pos.z,0.6)*sin(t*1.2+dot(cPos,vec3_splat(PI_HALF)));
  #endif
}

vec3 nlEntityLighting(nl_skycolor skycol,nl_environment env,vec3 pos,vec4 normal,vec3 wPos,mat4 world,vec4 tileLightCol,vec4 overlayCol,vec3 horizonEdgeCol,float t,float TIME_OF_DAY,float renderdistance,vec3 CAMERA_POS) {
  float l=tileLightCol.b;
  float tl=tileLightCol.r;
  float lum;
  vec3 light;
  if(env.nether||env.end) {
    tl=max(tl-0.6,0.0); tl*=21.0*tl;
    light=env.end?NL_END_AMBIENT:NL_NETHER_AMBIENT;
    light*=min(tileLightCol.b,0.25);
    if(env.end) light*=mix(NL_END_AMBIENT,skycol.horizon,0.35);
    lum=luminance(light);
    light+=skycol.horizon/(1.0+lum);
  } else {
    tl=max(tl-0.08,0.0); tl*=4.0*tl;
    float nightFactor=step(env.dayFactor,0.0);
    float dawnFactor=pow(max(1.0-env.dayFactor*env.dayFactor,0.0),3.0);
    dawnFactor*=mix(1.0,dawnFactor*dawnFactor,nightFactor);
    float sunLightAttenuation=clamp(0.5*(((2.0*step(TIME_OF_DAY,0.5)-1.0)*(wPos.x*cos(NL_SUN_PATH_YAW)+wPos.y*sin(NL_SUN_PATH_YAW))/renderdistance)+1.0),0.0,1.0);
    sunLightAttenuation=mix(1.0,sunLightAttenuation*sunLightAttenuation,dawnFactor);
    sunLightAttenuation*=1.0-0.55*env.rainFactor;
    light=(NL_SUNLIGHT_INTENSITY*l*sunLightAttenuation)*sunLightTint(env.dayFactor,env.rainFactor);
    vec3 N=normalize(mul(world,normal)).xyz;
    light*=0.86+max(N.y,0.0)*0.20;
    lum=luminance(light);
    light+=(skycol.horizon+skycol.zenith)*(l/(1.0+lum));
  }

  vec3 torchColor;
  if(env.underwater) torchColor=NL_UNDERWATER_TORCH_COL;
  else if(env.end) torchColor=NL_END_TORCH_COL;
  else if(env.nether) torchColor=NL_NETHER_TORCH_COL;
  else torchColor=NL_OVERWORLD_TORCH_COL;

  lum=luminance(light);
  light+=torchColor*(smoothstep(0.1,0.0,tileLightCol.b-tileLightCol.r)*NL_TORCHLIGHT_INTENSITY*tl/(1.0+lum));

  if(!(env.nether||env.end)) {
    lum=luminance(light);
    light+=vec3_splat(min(tileLightCol.r,0.15)*(NL_MIN_LIGHTING_BOOST/(1.0+lum)));
  }

  if(env.underwater) {
    vec3 gPos=wPos+CAMERA_POS;
    float c0=0.5+0.5*sin(dot(gPos,vec3(1.8,2.4,2.1))+0.8*t);
    float c1=0.5+0.5*sin(dot(gPos,vec3(-1.1,2.9,1.6))-1.1*t);
    float caustics=c0*c1;
    light+=0.7*NL_UNDERWATER_BRIGHTNESS+NL_CAUSTIC_INTENSITY*caustics*(0.1+tl);
    light*=mix(normalize(max(skycol.horizon,vec3_splat(0.001))),normalize(max(NL_UNDERWATER_TINT,vec3_splat(0.001))),tileLightCol.b*0.2);
  }

  lum=luminance(light);
  light+=vec3_splat(overlayCol.a*(1.6/(1.0+lum)));
  return light;
}

float nlEntityEdgeHighlight(vec4 edgemap) {
  #ifdef NL_ENTITY_EDGE_HIGHLIGHT
    vec2 len=min(abs(edgemap.xy),abs(edgemap.zw));
    len*=len;len*=len;
    float ambient=len.x+len.y*(1.0-len.x);
    return NL_ENTITY_BRIGHTNESS+ambient*NL_ENTITY_EDGE_HIGHLIGHT;
  #else
    return 1.0;
  #endif
}

vec4 nlEntityEdgeHighlightPreprocess(vec2 texcoord) {
  vec4 edgeMap=fract(vec4(texcoord*128.0,texcoord*256.0));
  return 2.0*step(edgeMap,vec4_splat(0.5))-1.0;
}

vec4 nlLavaNoise(vec3 gPos,float t) {
  float n=movingNoise2D(gPos.xz+gPos.yy,NL_LAVA_NOISE_SPEED*t,0.9);
  n=n*n;
  float veins=smoothstep(0.15,0.82,0.5+0.5*sin(4.0*gPos.x+3.0*gPos.z+2.0*t+n*4.0));
  vec3 lavaBase=normalize(max(NL_NETHER_AMBIENT,vec3_splat(0.001)));
  vec3 lavaHot=normalize(max(NL_NETHER_TORCH_COL,vec3_splat(0.001)));
  vec3 dark=mix(lavaBase,NL_NETHER_AMBIENT,n);
  vec3 hot=mix(NL_NETHER_TORCH_COL,lavaHot,n);
  vec3 col=mix(dark,hot,veins*n);
  col+=NL_NETHER_TORCH_COL*n*n;
  return vec4(col,n);
}

#endif
