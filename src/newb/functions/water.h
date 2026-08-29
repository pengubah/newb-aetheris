#ifndef WATER_H
#define WATER_H

#include "utils.h"
#include "detection.h"
#include "sky.h"
#include "clouds.h"
#include "noise.h"

// fresnel - Schlick's approximation
float calculateFresnel(float cosR, float r0) {
  float a = 1.0-cosR;
  float a2 = a*a;
  return r0 + (1.0-r0)*a2*a2*a;
}

vec4 nlWater(
  inout vec4 color, inout vec3 wPos, nl_skycolor skycol, nl_environment env, vec4 COLOR, vec3 viewDir,
  vec3 cPos, vec3 tiledCpos, vec3 gPos, vec3 CAMERA_POS, vec3 light, vec3 torchColor, vec2 lit,
  float fractCposY, float camDist, highp float t
) {

    vec2 bump = vec2(movingNoise2D(gPos.xz*0.72 + vec2(0.0,1.7) + gPos.yy*0.18, NL_WATER_WAVE_SPEED*t, 0.42), movingNoise2D(gPos.xz*0.56 + vec2(2.3,0.0) - gPos.yy*0.14, NL_WATER_WAVE_SPEED*t*0.82, 0.46));
  
  vec3 nrm;
  if (fractCposY > 0.0) { // top plane
    nrm.xz = bump*NL_WATER_BUMP;
    nrm.y = -1.0;
    /*if (fractCposY>0.8 || fractCposY<0.9) { // flat plane
    } else { // slanted plane and highly slanted plane
    }*/
  } else { // reflection for side plane
    bump *= 0.5 + 0.5*sin(3.0*t*NL_WATER_WAVE_SPEED + cPos.y*PI_HALF);
    nrm.xz = normalize(viewDir.xz) + bump.y*(1.0-viewDir.xz*viewDir.xz)*NL_WATER_BUMP;
    nrm.y = bump.x*NL_WATER_BUMP;
  }
  nrm = normalize(nrm);

  float cosR = dot(nrm, viewDir);
  viewDir = viewDir - 2.0*cosR*nrm ; // reflect(viewDir, nrm)

  vec3 waterRefl = nlRenderSky(skycol, env, viewDir, t, false);

  #if defined(NL_CLOUD_AURORA_REFLECTION)
    if (viewDir.y < 0.0) {
      vec4 cloudRefl = nlCloudAuroraReflection(skycol, env, viewDir, wPos, CAMERA_POS, t);
      waterRefl = mix(waterRefl, cloudRefl.rgb, cloudRefl.a);
    }
  #endif

  // torch light reflection
  float tc = 0.5+0.5*sin(16.0*viewDir.x)*sin(16.0*viewDir.z);
  waterRefl += torchColor*NL_TORCHLIGHT_INTENSITY*lit.x*tc*tc;

  // mask sky reflection under shade
  if (!env.end) {
    waterRefl *= 0.05 + lit.y*1.14;
  }

  #ifdef NL_WATER_REFL_MASK
    // Keep the reflection continuous across the water surface.
    // The previous sine-based mask could create a visible moving boundary.
    float reflAngle = abs(viewDir.y);
    float reflMask = smoothstep(0.0, 0.32, reflAngle);
    waterRefl *= mix(0.82, 1.0, reflMask);
  #endif

  cosR = abs(cosR);
  float fresnel = calculateFresnel(cosR, 0.07);
  float opacity = 1.0-cosR;

  color.rgb *= 0.22*NL_WATER_TINT*(1.0-0.8*fresnel);
  color.a = mix(COLOR.a*NL_WATER_TRANSPARENCY, 1.0, opacity*opacity);

    #ifdef NL_WATER_WAVE
  if (camDist < 14.0) {
    float time = NL_WATER_WAVE_SPEED*t;

    float wave1 = sin(gPos.x*0.82 + gPos.z*0.46 + time*0.82);
    float wave2 = sin(gPos.x*0.48 - gPos.z*0.91 + time*0.61);
    float wave3 = sin(gPos.x*1.35 + gPos.z*1.12 - time*1.08);
    float swell = wave1*0.50 + wave2*0.32 + wave3*0.18;
    float ripple = sin(gPos.x*2.8 - gPos.z*2.1 + time*1.35);
    swell += ripple*0.08;
    float waveShape = smoothstep(-0.75,0.75,swell);
    wPos.y -= waveShape*0.030*NL_WATER_BUMP;
    wPos.y -= (bump.x+bump.y)*0.010*NL_WATER_BUMP;
    
  }
  #endif

  return vec4(waterRefl, fresnel);
}

#endif
