#ifndef LIGHTING_H
#define LIGHTING_H

#include "detection.h"
#include "sky.h"
#include "utils.h"
#include "noise.h"
#include "clouds.h"

vec3 sunLightTint(float dayFactor, float rain) {
  float day = clamp(dayFactor,-1.0,1.0);
  float night = smoothstep(0.03,-0.38,day);
  float dawn = 1.0-day*day;
  dawn *= dawn;
  dawn *= dawn;
  dawn *= mix(1.0,dawn*dawn,night);
  vec3 tint = mix(NL_NOON_SUNLIGHT_COL,NL_NIGHT_MOONLIGHT_COL,night);
  tint = mix(tint,NL_DAWN_SUNLIGHT_COL,dawn*(1.0-0.35*night));
  float l = dot(tint,vec3_splat(0.333333));
  vec3 neutral = vec3_splat(l);
  tint = mix(tint,neutral,rain*0.18);
  return tint;
}

float nlSunAltitudeFactor(float dayFactor) {
  return smoothstep(-0.16,0.22,dayFactor);
}

float nlDawnLightFactor(float dayFactor) {
  float d = 1.0-dayFactor*dayFactor;
  d *= d;
  d *= d;
  return d;
}

vec3 nlLighting(
  sampler2D tex, nl_skycolor skycol, nl_environment env, vec3 wPos, out vec3 torchColor, vec3 COLOR,
  vec2 uv1, vec2 lit, bool isTree, float shade, highp float t, float renderdistance, float TIME_OF_DAY, vec3 CAMERA_POS
) {
  vec3 light;
  if (env.underwater) {
    torchColor = NL_UNDERWATER_TORCH_COL;
  } else if (env.end) {
    torchColor = NL_END_TORCH_COL;
  } else if (env.nether) {
    torchColor = NL_NETHER_TORCH_COL;
  } else {
    torchColor = NL_OVERWORLD_TORCH_COL;
  }

  float torchAttenuation = (NL_TORCHLIGHT_INTENSITY*uv1.x)/(0.5-0.45*lit.x);
  #ifdef NL_BLINKING_TORCH
    float flicker = 0.5+0.5*noise1D(t*7.5+dot(wPos,vec3_splat(0.17)));
    torchAttenuation *= 0.88+0.12*flicker;
  #endif
  vec3 torchLight = torchColor*torchAttenuation;
  float gameBrightness = texelFetch(tex,ivec2(0,0),0).g;
  float lum = 0.0;

  if (env.nether || env.end) {
    float endFactor = env.end ? 1.0 : 0.0;
    vec3 ambient = mix(NL_NETHER_AMBIENT,NL_END_AMBIENT,endFactor);
    float local = clamp(0.65+0.35*lit.y,0.0,1.0);
    ambient *= gameBrightness*(0.72+0.28*local);
    lum = luminance(ambient);
    vec3 skyAmbient = skycol.horizon*0.72+skycol.zenith*0.34;
    light = ambient+skyAmbient/(1.0+lum);
    if (env.end) {
      float voidDepth = smoothstep(0.0,1.0,1.0-lit.y);
      light *= 0.82+0.18*lit.y;
      light += skycol.horizon*(0.18+0.22*voidDepth);
    }
  } else {
    float night = smoothstep(0.03,-0.35,env.dayFactor);
    float dawn = nlDawnLightFactor(env.dayFactor);
    float altitude = nlSunAltitudeFactor(env.dayFactor);
    float nightIntensity = night*night;
    float sunAtt = clamp(0.5*(((2.0*step(TIME_OF_DAY,0.5)-1.0)*(wPos.x*cos(NL_SUN_PATH_YAW)+wPos.y*sin(NL_SUN_PATH_YAW))/max(renderdistance,1.0))+1.0),0.0,1.0);
    sunAtt = mix(sunAtt*sunAtt,1.0,dawn*0.72);
    sunAtt *= mix(1.0,0.58,env.rainFactor);
    sunAtt *= 0.42+0.58*altitude;

    float shadow = step(0.93,uv1.y);
    float shadowFloor = 1.0-NL_SHADOW_INTENSITY+(0.58*NL_SHADOW_INTENSITY*nightIntensity);
    shadow = max(shadow,shadowFloor*lit.y);
    shadow *= shade>0.8 ? 1.0 : 0.78;

    #if defined(NL_CLOUD_SHADOW) && (NL_CLOUD_TYPE == 1 || NL_CLOUD_TYPE == 2)
      vec3 mainLightDir = env.sunDir.y>0.0 ? env.sunDir : env.moonDir;
      vec3 gPos = wPos+CAMERA_POS;
      float cloudRelativeHeight = gPos.y-187.0;
      float safeY = mainLightDir.y>=0.03 ? mainLightDir.y : 0.03;
      vec2 projectionOffset = cloudRelativeHeight*mainLightDir.xz/safeY;
      vec2 projectedPos = gPos.xz+projectionOffset;
      float cloudFade = smoothstep(1.0,0.5,length(0.002*(wPos.xz+projectionOffset)));
      cloudFade *= (1.0-dawn*dawn)*clamp(-0.12*(cloudRelativeHeight-7.0),0.0,1.0);
      float cmask;
      #if NL_CLOUD_TYPE == 1
        cmask = cloudNoise2D(projectedPos*NL_CLOUD1_SCALE,t,env.rainFactor)*cloudFade;
      #elif NL_CLOUD_TYPE == 2
        projectedPos = NL_CLOUD2_SCALE*(projectedPos+vec2(1.0,0.5)*(t*NL_CLOUD2_VELOCITY));
        cmask = cloudDf(vec3(projectedPos.x,0.5,projectedPos.y),env.rainFactor,NL_CLOUD2_SHAPE)*cloudFade;
      #endif
      shadow *= mix(1.0,0.32,smoothstep(0.38,0.88,cmask));
    #endif

    vec3 directTint = sunLightTint(env.dayFactor,env.rainFactor);
    light = NL_SUNLIGHT_INTENSITY*shadow*sunAtt*directTint;
    lum = luminance(light);
    float skyWeight = uv1.y*(0.68+0.32*lit.y);
    vec3 skyAmbient = mix(skycol.horizon,skycol.zenith,0.45+0.35*uv1.y);
    light += skyAmbient*(skyWeight/(1.0+lum));
    light += skycol.horizon*(0.10+0.18*(1.0-uv1.y))*dawn;
  }

  lum = luminance(light);
  light += torchLight/(1.0+lum);

  if (!(env.nether || env.end)) {
    lum = luminance(light);
    light += vec3_splat(gameBrightness*(NL_MIN_LIGHTING_BOOST/(1.0+lum)));
  }

  light *= COLOR.g>0.35 ? 1.0 : 0.80;
  if (isTree) light *= 1.18+0.10*lit.y;
  return max(light,vec3_splat(0.0));
}

void nlUnderwaterLighting(inout vec3 light, inout vec3 pos, vec2 lit, vec2 uv1, vec3 tiledCpos, vec3 cPos, highp float t, vec3 horizonCol) {
  if (uv1.y<0.9) {
    float caustics = disp(tiledCpos,NL_WATER_WAVE_SPEED*t);
    caustics *= 3.0*caustics;
    light += NL_UNDERWATER_BRIGHTNESS+NL_CAUSTIC_INTENSITY*caustics*(0.15+lit.y+lit.x*0.7);
  }
  float waterL = max(luminance(horizonCol),0.001);
  vec3 waterTint = horizonCol/waterL;
  light *= mix(waterTint,vec3_splat(0.6),lit.y*0.62);
  #ifdef NL_UNDERWATER_WAVE
    pos.xy += NL_UNDERWATER_WAVE*min(0.05*pos.z,0.6)*sin(t*1.2+dot(cPos,vec3_splat(PI_HALF)));
  #endif
}

vec3 nlEntityLighting(nl_skycolor skycol, nl_environment env, vec3 pos, vec4 normal, vec3 wPos, mat4 world, vec4 tileLightCol, vec4 overlayCol, vec3 horizonEdgeCol, float t, float TIME_OF_DAY, float renderdistance, vec3 CAMERA_POS) {
  float l = tileLightCol.b;
  float tl = tileLightCol.r;
  float lum;
  vec3 light;
  if (env.nether || env.end) {
    tl = max(tl-0.6,0.0);
    tl *= 21.0*tl;
    light = mix(NL_NETHER_AMBIENT,NL_END_AMBIENT,env.end ? 1.0 : 0.0);
    light *= min(tileLightCol.b,0.25)*(0.78+0.22*l);
    lum = luminance(light);
    light += skycol.horizon/(1.0+lum);
    if (env.end) light += skycol.zenith*(0.18+0.18*(1.0-l));
  } else {
    tl = max(tl-0.08,0.0);
    tl *= 4.0*tl;
    float night = smoothstep(0.03,-0.35,env.dayFactor);
    float dawn = nlDawnLightFactor(env.dayFactor);
    float sunAtt = clamp(0.5*(((2.0*step(TIME_OF_DAY,0.5)-1.0)*(wPos.x*cos(NL_SUN_PATH_YAW)+wPos.y*sin(NL_SUN_PATH_YAW))/max(renderdistance,1.0))+1.0),0.0,1.0);
    sunAtt = mix(sunAtt*sunAtt,1.0,dawn*0.72);
    sunAtt *= 1.0-0.5*env.rainFactor;
    vec3 tint = sunLightTint(env.dayFactor,env.rainFactor);
    light = NL_SUNLIGHT_INTENSITY*l*sunAtt*tint;
    vec3 N = normalize(mul(world,normal)).xyz;
    light *= 0.82+max(N.y,0.0)*0.42;
    lum = luminance(light);
    light += (skycol.horizon+skycol.zenith)*(l/(1.0+lum));
    light += skycol.horizon*(0.08+0.14*dawn+0.08*night);
  }

  vec3 torchColor;
  if (env.underwater) torchColor = NL_UNDERWATER_TORCH_COL;
  else if (env.end) torchColor = NL_END_TORCH_COL;
  else if (env.nether) torchColor = NL_NETHER_TORCH_COL;
  else torchColor = NL_OVERWORLD_TORCH_COL;

  lum = luminance(light);
  light += torchColor*(smoothstep(0.1,0.0,tileLightCol.b-tileLightCol.r)*NL_TORCHLIGHT_INTENSITY*tl/(1.0+lum));
  lum = luminance(light);
  if (!(env.nether || env.end)) light += vec3_splat(min(tileLightCol.r,0.15)*(NL_MIN_LIGHTING_BOOST/(1.0+lum)));

  if (env.underwater) {
    vec3 gPos = wPos+CAMERA_POS;
    float caustics = 0.5+0.5*sin(dot(gPos,vec3(1.8,2.4,2.1))+0.8*t);
    caustics *= 0.65+0.35*sin(dot(gPos,vec3(-1.1,1.7,2.8))-1.1*t);
    light += 0.8*NL_UNDERWATER_BRIGHTNESS+NL_CAUSTIC_INTENSITY*caustics*(0.1+tl);
    light *= mix(normalize(skycol.horizon),vec3_splat(0.5),tileLightCol.b*0.2);
  }

  lum = luminance(light);
  light += vec3_splat(overlayCol.a*(1.5/(1.0+lum)));
  return max(light,vec3_splat(0.0));
}

float nlEntityEdgeHighlight(vec4 edgemap) {
  #ifdef NL_ENTITY_EDGE_HIGHLIGHT
    vec2 len = min(abs(edgemap.xy),abs(edgemap.zw));
    len *= len; len *= len;
    float ambient = len.x+len.y*(1.0-len.x);
    return NL_ENTITY_BRIGHTNESS+ambient*NL_ENTITY_EDGE_HIGHLIGHT;
  #else
    return 1.0;
  #endif
}

vec4 nlEntityEdgeHighlightPreprocess(vec2 texcoord) {
  vec4 edgeMap = fract(vec4(texcoord*128.0,texcoord*256.0));
  return 2.0*step(edgeMap,vec4_splat(0.5))-1.0;
}

vec4 nlLavaNoise(vec3 gPos, float t) {
  float n = movingNoise2D(gPos.xz+gPos.yy,NL_LAVA_NOISE_SPEED*t,0.9);
  n *= n;
  float edge = smoothstep(0.08,0.88,n);
  vec3 base = mix(vec3(0.7,0.4,0.0),vec3_splat(1.5),edge*edge);
  return vec4(base,n);
}

#endif
