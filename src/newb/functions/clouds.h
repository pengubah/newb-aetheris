#ifndef CLOUDS_H
#define CLOUDS_H

#include "detection.h"
#include "noise.h"
#include "sky.h"

float cloudNoise2D(vec2 p, highp float t, float rain) {
  t*=NL_CLOUD1_SPEED;
  vec2 drift=vec2(1.0,0.32)*t;
  p+=drift;
  p.y+=2.2*sin(0.22*p.x+0.7*t);
  float base=noise2D(p);
  float detail=noise2D(p*2.15+vec2(4.1,-2.7)-0.15*t);
  float billow=noise2D(p*0.52+vec2(9.0,3.0)+0.05*t);
  float shape=base*0.62+detail*0.25+billow*0.13;
  float threshold=mix(0.58,0.48,rain);
  shape=smoothstep(threshold-0.14,threshold+0.12,shape);
  shape*=0.92+0.08*sin(p.x*0.45+t);
  return shape*shape;
}

vec4 renderCloudsSimple(nl_skycolor skycol, vec3 pos, highp float t, float rain) {
  pos.xz*=NL_CLOUD1_SCALE;
  float d=cloudNoise2D(pos.xz,t,rain);
  float edge=smoothstep(0.06,0.76,d);
  vec3 base=mix(skycol.horizon,skycol.zenith,0.35+0.25*edge);
  float light=luminance(base);
  vec3 col=base*(0.82+0.38*light)+0.16*skycol.horizonEdge;
  col*=mix(1.0,0.62,rain);
  col+=0.12*skycol.horizonEdge*(1.0-rain)*smoothstep(0.65,1.0,d);
  return vec4(col,edge*NL_CLOUD1_OPACITY);
}

float cloudDf(vec3 pos, float rain, vec2 boxiness) {
  vec2 cell=floor(pos.xz);
  vec2 f=fract(pos.xz);
  vec2 shape=mix(vec2(0.06),boxiness,0.55);
  vec2 u=smoothstep(shape,1.0-shape,f);
  vec4 r=vec4(rand(cell),rand(cell+vec2(1.0,0.0)),rand(cell+vec2(1.0,1.0)),rand(cell+vec2(0.0,1.0)));
  r=smoothstep(0.12+0.18*rain,0.72+0.05*rain,r);
  float n=mix(mix(r.x,r.y,u.x),mix(r.w,r.z,u.x),u.y);
  float vertical=1.0-smoothstep(0.1,1.0,abs(pos.y-0.5)*(1.2+boxiness.y));
  float rainLift=1.0-0.16*rain;
  n*=vertical*rainLift;
  n=max(1.35*(n-0.16),0.0);
  return n*n*(3.0-2.0*n);
}

vec4 renderCloudsRounded(
    vec3 vDir, vec3 vPos, float rain, float time, vec3 horizonCol, vec3 zenithCol,
    const int steps, const float thickness, const float thickness_rain, const float speed,
    const vec2 scale, const float density, const vec2 boxiness
) {
  float height=7.0*mix(thickness,thickness_rain,rain);
  float stepsf=float(steps);
  vec3 deltaP;
  deltaP.y=1.0;
  deltaP.xz=height*scale*vDir.xz/(0.035+0.965*abs(vDir.y));
  vec3 pos;
  pos.y=0.0;
  pos.xz=scale*(vPos.xz+vec2(1.0,0.5)*(time*speed));
  pos+=deltaP;
  deltaP/=-stepsf;

  vec2 accum=vec2(0.0,1.0);
  for(int i=1;i<=steps;i++) {
    float m=cloudDf(pos,rain,boxiness);
    float edge=1.0-abs(pos.y-0.5)*1.8;
    accum.x+=m*(0.55+0.45*saturate(edge));
    accum.y=mix(accum.y,pos.y,m);
    pos+=deltaP;
  }
  accum.x*=smoothstep(0.015,0.10,accum.x);
  accum.x/=stepsf/max(density,0.001)+accum.x;
  if(vPos.y<0.0) accum.y=1.0-accum.y;

  float topLight=saturate(1.0-accum.y);
  vec3 col=mix(horizonCol,zenithCol,0.28+0.48*topLight);
  col*=mix(1.08,0.64,rain);
  col+=0.14*zenithCol*topLight*(1.0-rain);
  return vec4(col,accum.x);
}

float cloudsNoiseVr(vec2 p, float t) {
  float a=fastVoronoi2(p+t,1.65);
  float b=fastVoronoi2(2.4*p-0.35*t,1.25);
  float c=fastVoronoi2(6.5*p+0.17*t,0.65);
  float d=fastVoronoi2(18.0*p-0.08*t,0.20);
  return saturate((a*0.48+b*0.30+c*0.16+d*0.06));
}

vec4 renderClouds(vec2 p, float t, float rain, vec3 horizonCol, vec3 zenithCol, const vec2 scale, const float velocity, const float shadow) {
  p*=scale;
  t*=velocity;
  float a=cloudsNoiseVr(p,t);
  float b=cloudsNoiseVr(p+NL_CLOUD3_SHADOW_OFFSET*scale,t+0.37);
  vec2 p2=1.37*p.yx+vec2(7.8,9.2);
  float c=cloudsNoiseVr(p2,0.55*t+1.7);
  float d=cloudsNoiseVr(p2+NL_CLOUD3_SHADOW_OFFSET*scale,0.55*t+2.2);

  vec2 tr=vec2(0.46,0.68)-0.08*rain;
  a=smoothstep(tr.x,tr.y,a);
  c=smoothstep(tr.x+0.02,tr.y+0.04,c);
  b=smoothstep(0.20,0.82,b);
  d=smoothstep(0.20,0.82,d);

  float alpha=saturate(a+c*(1.0-a)*0.72);
  float shade=saturate(mix(b,d,c)*shadow);
  vec3 sunlit=mix(zenithCol,horizonCol,0.45+0.25*rain);
  vec3 shadowed=0.42*zenithCol+0.32*horizonCol;
  vec3 col=mix(sunlit,shadowed,shade);
  col*=mix(1.0,0.56,rain);
  col+=0.10*zenithCol*(1.0-shade)*(1.0-rain);
  return vec4(col,alpha);
}

#ifdef NL_AURORA
vec4 renderAurora(vec3 p, float t, float rain, vec3 FOG_COLOR) {
  t*=NL_AURORA_VELOCITY;
  p.xz*=NL_AURORA_SCALE;
  float wave=sin(p.x*3.5+t)+0.55*sin(p.z*2.1-t*0.7);
  p.xz+=0.06*vec2(sin(wave+p.z),cos(wave+p.x));
  float d0=sin(p.x*0.13+t+sin(p.z*0.22));
  float d1=sin(p.z*0.11-t+sin(p.x*0.18));
  float d2=sin(p.z*0.12+d0*1.7+d1*2.1);
  float ribbon=d0*d0/(1.0+d2*d2/max(NL_AURORA_WIDTH,0.001));
  float horizonMask=smoothstep(-0.25,0.55,p.y);
  float fogMask=1.0-saturate(0.55*max(FOG_COLOR.b,FOG_COLOR.g));
  float mask=(1.0-0.86*rain)*fogMask*horizonMask;
  vec3 col=mix(NL_AURORA_COL1,NL_AURORA_COL2,smoothstep(-0.5,0.8,d1));
  return vec4(col*NL_AURORA,ribbon*mask);
}
#endif

vec4 nlCloudAuroraReflection(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 wPos, vec3 CAMERA_POS, highp float t) {
  vec2 cloudPos=wPos.xz;
  float vy=max(abs(viewDir.y),0.05);
  cloudPos+=(187.0-(wPos.y+CAMERA_POS.y))*viewDir.xz/vy;
  float fade=saturate(2.2-0.005*length(cloudPos));
  cloudPos+=CAMERA_POS.xz;
  vec4 refl=vec4_splat(0.0);

  #ifdef NL_AURORA
    vec4 aurora=renderAurora(cloudPos.xyy,t,env.rainFactor,env.fogCol);
    aurora.a*=fade;
    refl=vec4(1.8*aurora.rgb*aurora.a,aurora.a);
  #endif
  #if NL_CLOUD_TYPE == 1
    vec4 clouds=renderCloudsSimple(skycol,cloudPos.xyy,t,env.rainFactor);
    clouds.a*=fade;
    refl=vec4(mix(refl.rgb,clouds.rgb,clouds.a),min(refl.a+clouds.a,1.0));
  #endif
  return refl;
}

#endif
