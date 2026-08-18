#ifndef GLOW_H
#define GLOW_H

#include "sky.h"

vec3 glowDetect(vec4 diffuse) {
  float a=diffuse.a;
  if(a>0.986 && a<0.995) {
    float mask=smoothstep(0.986,0.991,a);
    vec3 base=max(diffuse.rgb,vec3_splat(0.0));
    vec3 glow=base*base;
    glow*=0.72+0.48*mask;
    return glow;
  }
  return vec3_splat(0.0);
}

vec3 glowDetectC(sampler2D tex, vec2 uv) { return glowDetect(texture2DLod(tex,uv,0.0)); }

vec3 glowDetectD(sampler2D tex, vec2 uv, vec2 offset, vec2 st) {
  vec2 safeOffset=offset*step(offset*st,vec2_splat(0.0));
  return glowDetectC(tex,uv+safeOffset);
}

vec3 nlGlow(sampler2D tex, vec2 uv, float shimmer) {
  vec3 glow=glowDetectC(tex,uv);
  #ifdef NL_GLOW_LEAK
    vec2 texSize=vec2(textureSize(tex,0));
    vec2 offset=1.0/max(texSize,vec2_splat(1.0));
    float boundSize=texSize.x/64.0;
    vec2 st=boundSize*fract(64.0*uv*(texSize/texSize.x))-0.5*boundSize;
    st=sign(st)*step(vec2_splat(0.5*boundSize-1.01),abs(st));
    vec3 c1=glowDetectD(tex,uv,offset*vec2(-1,-1),st);
    vec3 c2=glowDetectD(tex,uv,offset*vec2(-1,0),st);
    vec3 c3=glowDetectD(tex,uv,offset*vec2(-1,1),st);
    vec3 c4=glowDetectD(tex,uv,offset*vec2(0,1),st);
    vec3 c5=glowDetectD(tex,uv,offset*vec2(1,1),st);
    vec3 c6=glowDetectD(tex,uv,offset*vec2(1,0),st);
    vec3 c7=glowDetectD(tex,uv,offset*vec2(1,-1),st);
    vec3 c8=glowDetectD(tex,uv,offset*vec2(0,-1),st);
    vec2 u=fract(uv*texSize);
    vec3 corners=mix(mix(c1,c3,u.y),mix(c7,c5,u.y),u.x);
    vec3 sides=max(max(c2*(1.0-u.x),c4*u.y),max(c6*u.x,c8*(1.0-u.y)));
    vec3 leak=(0.58*corners+0.42*sides);
    leak=((leak*0.8+0.16)*leak+0.05)*leak;
    glow=max(glow,leak*NL_GLOW_LEAK);
  #endif
  #ifdef NL_GLOW_SHIMMER
    glow*=max(shimmer,0.0);
  #endif
  return glow*NL_GLOW_TEX;
}

#ifdef NL_GLOW_SHIMMER
float nlGlowShimmer(vec3 cPos, float t) {
  float p=dot(cPos,vec3(1.0,0.7,1.2));
  float a=0.5+0.5*sin(0.72*p-NL_GLOW_SHIMMER_SPEED*t);
  float b=0.5+0.5*sin(1.31*p+0.63*t+2.0*a);
  float shimmer=0.45*a+0.55*b;
  shimmer=shimmer*shimmer;
  return mix(1.0,0.72+0.56*shimmer,NL_GLOW_SHIMMER);
}
#endif

vec4 nlGlint(vec4 light, vec4 layerUV, sampler2D glintTexture, vec4 glintColor, vec4 tileLightColor, vec4 albedo) {
  float d=fract(dot(albedo.rgb,vec3_splat(4.0)));
  vec4 tex1=texture2D(glintTexture,fract(layerUV.xy+0.08*d)).rgbr;
  vec4 tex2=texture2D(glintTexture,fract(layerUV.zw-0.06*d)).rgbr;
  vec4 glint=(tex1*tex1+tex2*tex2)*tileLightColor*glintColor;
  float intensity=saturate(glint.a);
  light.rgb=mix(light.rgb,light.rgb+glint.rgb*24.0,0.35*intensity);
  light.rgb+=0.035*spectrum(sin(layerUV.x*9.42477+2.0*intensity+d));
  return light;
}

#endif
