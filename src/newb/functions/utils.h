#ifndef UTILS_H
#define UTILS_H

#define PI 3.141592
#define PI_HALF 1.570796
#define PI_QUART 0.785398

mat2 rmat2(float t) {
  float sint = sin(t);
  float cost = cos(t);
  return mtxFromRows(vec2(cost, -sint), vec2(sint, cost));
}

float degToRad(float t) { return 0.0174533*t; }

float luminance(vec3 x) { return dot(x, vec3(0.21, 0.71, 0.08)); }

float nlSaturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 nlSaturate3(vec3 x) { return clamp(x, vec3_splat(0.0), vec3_splat(1.0)); }

float nlSoftLight(float x, float y) {
  return mix(x, x*y*2.0 + x*x*(1.0-2.0*y), step(0.5,y));
}

float nlHash(float n) { return fract(sin(n)*43758.5453); }

float nlHash2(vec2 p) {
  return fract(sin(dot(p, vec2(127.1,311.7)))*43758.5453);
}

vec2 nlHash22(vec2 p) {
  return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

float nlPulse(float x, float center, float width) {
  return 1.0-smoothstep(0.0,width,abs(x-center));
}

float nlContrast(float x, float c) {
  return clamp((x-0.5)*c+0.5,0.0,1.0);
}

#endif
