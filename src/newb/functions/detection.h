#ifndef DETECTION_H
#define DETECTION_H

#include "utils.h"

struct nl_environment {
  bool end;
  bool nether;
  bool underwater;
  float rainFactor;
  float dayFactor;
  vec3 sunDir;
  vec3 moonDir;
  vec3 fogCol;
};

bool detectEnd(float DIMENSION_ID) {
  return abs(DIMENSION_ID-2.0)<0.01;
}

bool detectNether(float DIMENSION_ID, vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  float warm=FOG_COLOR.r-max(FOG_COLOR.g,FOG_COLOR.b);
  bool underLava=FOG_CONTROL.x<0.01 && FOG_COLOR.b<0.03 && FOG_COLOR.g<0.22 && warm>0.08;
  return abs(DIMENSION_ID-1.0)<0.01 || underLava;
}

bool detectUnderwater(vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  float blueBias=FOG_COLOR.b-max(FOG_COLOR.r,FOG_COLOR.g)*0.82;
  float greenBias=FOG_COLOR.g-FOG_COLOR.r*0.92;
  return FOG_CONTROL.x<0.01 && FOG_CONTROL.y<0.82 && max(blueBias,greenBias)>0.015;
}

float detectRain(vec3 FOG_CONTROL) {
  float z=max(FOG_CONTROL.z,16.0);
  float clearX=0.5+20.0/z;
  float clearY=1.0;
  float rx=remap01(FOG_CONTROL.x,clearX,0.23);
  float ry=remap01(FOG_CONTROL.y,clearY,0.70);
  float val=saturate(rx*ry);
  val=softLightCurve(val);
  return val*val*(3.0-2.0*val);
}

float detectDayFactor(vec3 FOG_COLOR) {
  float v=dot(max(FOG_COLOR,vec3_splat(0.0)),vec3(0.48,0.70,0.50));
  return saturate(v);
}

nl_environment calculateSunParams(nl_environment env, float TIME_OF_DAY, float DAY) {
  float t=2.0*PI*TIME_OF_DAY;
  vec3 sunDir=vec3(sin(t),cos(t),0.0);
  vec3 moonDir=-sunDir;
  env.dayFactor=sunDir.y;

  sunDir.yz=mul(rmat2(-degToRad(NL_SUN_PATH_TILT)),sunDir.yz);
  sunDir.xz=mul(rmat2(degToRad(NL_SUN_PATH_YAW)),sunDir.xz);
  moonDir.yz=mul(rmat2(-degToRad(NL_MOON_PATH_TILT)),moonDir.yz);
  moonDir.xz=mul(rmat2(degToRad(NL_MOON_PATH_YAW)),moonDir.xz);

  env.sunDir=safeNormalize(sunDir);
  env.moonDir=safeNormalize(moonDir);
  return env;
}

nl_environment nlDetectEnvironment(float DIMENSION_ID, float TIME_OF_DAY, float DAY, vec3 FOG_COLOR, vec3 FOG_CONTROL) {
  nl_environment env;
  env.end=false;
  env.nether=false;
  env.underwater=false;
  env.rainFactor=0.0;
  env.dayFactor=0.0;
  env.sunDir=vec3(0.0,1.0,0.0);
  env.moonDir=vec3(0.0,-1.0,0.0);
  env.fogCol=FOG_COLOR;

  env.end=detectEnd(DIMENSION_ID);
  env.nether=detectNether(DIMENSION_ID,FOG_COLOR,FOG_CONTROL.xy);
  env.underwater=!env.nether && !env.end && detectUnderwater(FOG_COLOR,FOG_CONTROL.xy);
  env.rainFactor=detectRain(FOG_CONTROL);
  env.fogCol=FOG_COLOR;
  env=calculateSunParams(env,TIME_OF_DAY,DAY);
  return env;
}

#endif
