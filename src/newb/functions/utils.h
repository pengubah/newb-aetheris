#ifndef UTILS_H
#define UTILS_H

#define PI 3.141592
#define PI_HALF 1.570796
#define PI_QUART 0.785398

mat2 rmat2(float t) {
  float s = sin(t);
  float c = cos(t);
  return mtxFromRows(vec2(c, -s), vec2(s, c));
}

float degToRad(float t) { return 0.0174533*t; }

float luminance(vec3 x) { return dot(max(x, vec3_splat(0.0)), vec3(0.2126,0.7152,0.0722)); }
vec3 saturate3(vec3 x) { return clamp(x, vec3_splat(0.0), vec3_splat(1.0)); }
float softLightCurve(float x) { x = saturate(x); return x*x*(3.0-2.0*x); }
float remap01(float x, float a, float b) { return saturate((x-a)/max(b-a,0.0001)); }
vec3 safeNormalize(vec3 v) { return v*inversesqrt(max(dot(v,v),0.000001)); }
float smoothPulse(float x, float a, float b) { float p=smoothstep(a,b,x); return p*(1.0-p)*4.0; }

#endif
