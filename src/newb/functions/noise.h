#ifndef NOISE_H
#define NOISE_H

#include "utils.h"

float rand(highp vec2 n) {
  return fract(sin(dot(n, vec2(12.9898,4.1414))) * 43758.5453);
}

float noise1D(highp float x) {
  float x0=floor(x);
  float f=x-x0;
  f=f*f*(3.0-2.0*f);
  float a=fract(sin(x0)*84.85);
  float b=fract(sin(x0+1.0)*84.85);
  return mix(a,b,f);
}

float fastRand(vec2 n){
  vec2 p=fract(n*vec2(0.1031,0.1030));
  p+=dot(p,p.yx+33.33);
  return fract((p.x+p.y)*p.x);
}

float disp(vec3 pos, float t) {
  float wave=sin(8.0*PI_HALF*(pos.x+pos.y*pos.z)+0.7*t);
  float wave2=sin(5.0*(pos.z-pos.x)+1.1*t+wave);
  pos.y+=t+0.65*wave+0.35*wave2;
  float cell=floor(pos.y);
  float f=fract(pos.y);
  f=f*f*(3.0-2.0*f);
  float a=fastRand(pos.xz+cell);
  float b=fastRand(pos.xz+cell+1.0);
  return (0.72+0.28*(0.5+0.5*wave2))*mix(a,b,f);
}

float noise2D(vec2 u) {
  vec2 i=floor(u);
  vec2 f=fract(u);
  f=f*f*(3.0-2.0*f);
  float c0=rand(i);
  float c1=rand(i+vec2(1.0,0.0));
  float c2=rand(i+vec2(1.0,1.0));
  float c3=rand(i+vec2(0.0,1.0));
  return mix(mix(c0,c1,f.x),mix(c3,c2,f.x),f.y);
}

vec4 mod289(vec4 x) { return x-floor(x*(1.0/289.0))*289.0; }
vec4 perm(vec4 x) { return mod289(((x*34.0)+1.0)*x); }

float noise3D(vec3 p){
  vec3 a=floor(p);
  vec3 d=fract(p);
  d=d*d*(3.0-2.0*d);
  vec4 b=a.xxyy+vec4(0.0,1.0,0.0,1.0);
  vec4 k1=perm(b.xyxy);
  vec4 k2=perm(k1.xyxy+b.zzww);
  vec4 c=k2+a.zzzz;
  vec4 k3=perm(c);
  vec4 k4=perm(c+1.0);
  vec4 o1=fract(k3/41.0);
  vec4 o2=fract(k4/41.0);
  vec4 o3=mix(o1,o2,d.z);
  vec2 o4=mix(o3.xz,o3.yw,d.x);
  return mix(o4.x,o4.y,d.y);
}

float fastVoronoi2(vec2 pos, float f) {
  vec2 cell=floor(pos);
  vec2 p=fract(pos)-0.5;
  float n=0.0;
  float best=10.0;
  for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
    vec2 o=vec2(float(x),float(y));
    vec2 r=vec2(rand(cell+o),rand(cell+o+vec2(7.1,3.7)))-0.5;
    vec2 q=o+r-p;
    best=min(best,dot(q,q));
  }
  n=1.0-saturate(sqrt(best)*f);
  return n;
}

float movingNoise2D(vec2 pos, float t, float f) {
  vec2 q=pos+vec2(0.37,-0.21)*t;
  float n0=noise2D(q*0.35+vec2(0.0,t*0.07));
  float n1=noise2D(q*0.85-vec2(t*0.11,0.0));
  float n2=fastVoronoi2(q*0.55+0.25*t,1.8);
  float detail=noise2D(q*1.7-0.18*t);
  float base=mix(n0,n1,0.5+0.5*sin(t*0.17));
  return mix(base,base*0.65+0.35*n2,clamp(f,0.0,1.0))*0.88+0.12*detail;
}

#endif
