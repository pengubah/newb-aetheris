#ifndef WATER_H
#define WATER_H

#include "utils.h"
#include "detection.h"
#include "sky.h"
#include "clouds.h"
#include "noise.h"

float calculateFresnel(float cosR, float r0) {
  float a = 1.0-cosR;
  float a2 = a*a;
  return r0+(1.0-r0)*a2*a2*a;
}

vec4 nlWater(
  inout vec4 color, inout vec3 wPos, nl_skycolor skycol, nl_environment env, vec4 COLOR, vec3 viewDir,
  vec3 cPos, vec3 tiledCpos, vec3 gPos, vec3 CAMERA_POS, vec3 light, vec3 torchColor, vec2 lit,
  float fractCposY, float camDist, highp float t
) {
  vec2 bump = vec2_splat(movingNoise2D(gPos.xz+gPos.yy,NL_WATER_WAVE_SPEED*t,0.6));
  bump += 0.35*vec2(movingNoise2D(gPos.xz+vec2(7.0,-3.0),NL_WATER_WAVE_SPEED*0.7*t,0.55),movingNoise2D(gPos.zx+vec2(-4.0,8.0),NL_WATER_WAVE_SPEED*0.55*t,0.55));
  bump = clamp(bump,-1.0,1.0);

  vec3 nrm;
  if (fractCposY>0.0) {
    nrm.xz = bump*NL_WATER_BUMP;
    nrm.y = -1.0;
  } else {
    bump *= 0.5+0.5*sin(3.0*t*NL_WATER_WAVE_SPEED+cPos.y*PI_HALF);
    nrm.xz = normalize(viewDir.xz)+bump.y*(1.0-viewDir.xz*viewDir.xz)*NL_WATER_BUMP;
    nrm.y = bump.x*NL_WATER_BUMP;
  }
  nrm = normalize(nrm);
  float cosR = dot(nrm,viewDir);
  viewDir = viewDir-2.0*cosR*nrm;
  vec3 waterRefl = nlRenderSky(skycol,env,viewDir,t,false);

  #if defined(NL_CLOUD_AURORA_REFLECTION)
    if (viewDir.y<0.0) {
      vec4 cloudRefl = nlCloudAuroraReflection(skycol,env,viewDir,wPos,CAMERA_POS,t);
      waterRefl = mix(waterRefl,cloudRefl.rgb,cloudRefl.a);
    }
  #endif

  float sparkle = movingNoise2D(gPos.xz+gPos.yy,1.4*t,0.4);
  sparkle *= sparkle*smoothstep(0.72,0.98,viewDir.y);
  waterRefl += torchColor*NL_TORCHLIGHT_INTENSITY*lit.x*(0.25+0.75*sparkle);

  if (!env.end) waterRefl *= 0.05+lit.y*1.14;

  #ifdef NL_WATER_REFL_MASK
    float mask = 0.05+0.05*sin(viewDir.x*12.0)*sin(viewDir.z*6.0);
    waterRefl *= smoothstep(mask-0.2,mask+0.13,viewDir.y*viewDir.y);
  #endif

  cosR = abs(cosR);
  float fresnel = calculateFresnel(cosR,0.07);
  float grazing = smoothstep(0.2,0.95,1.0-cosR);
  float opacity = 1.0-cosR;
  float depthTint = mix(0.72,1.12,lit.y);
  color.rgb *= 0.22*NL_WATER_TINT*(1.0-0.8*fresnel)*depthTint;
  color.a = mix(COLOR.a*NL_WATER_TRANSPARENCY,1.0,opacity*opacity);
  color.a = max(color.a,fresnel*0.32+grazing*0.08);

  #ifdef NL_WATER_WAVE
    if (camDist<14.0) wPos.y -= 0.5*(bump.x+0.5)*NL_WATER_BUMP;
  #endif

  return vec4(waterRefl,fresnel);
}

#endif
