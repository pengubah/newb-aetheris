#ifndef FOG_H
#define FOG_H

float nlFogWarmMask(vec3 c) {
  float chroma = max(max(c.r,c.g),c.b)-min(min(c.r,c.g),c.b);
  return smoothstep(0.025,0.42,max(c.r-c.b,c.r-c.g)*1.8)*smoothstep(0.0,0.8,chroma+0.02);
}

float nlFogCoolMask(vec3 c) {
  return smoothstep(0.02,0.35,max(c.b-c.r,c.g-c.r)*1.5);
}

float nlRenderFogFade(float relativeDist, vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float base = smoothstep(FOG_CONTROL.x,FOG_CONTROL.y,relativeDist);
    float distNorm = clamp((relativeDist-FOG_CONTROL.x)/(FOG_CONTROL.y-FOG_CONTROL.x+0.0001),0.0,1.0);
    float warm = nlFogWarmMask(FOG_COLOR);
    float cool = nlFogCoolMask(FOG_COLOR);
    float lum = dot(FOG_COLOR,vec3(0.21,0.71,0.08));
    float density = NL_MIST_DENSITY*(13.0+10.0*(1.0-lum));
    float mist = 1.0-exp(-relativeDist*relativeDist*0.0007*density);
    float nearMist = smoothstep(0.0,0.55,distNorm)*mist;
    float sunsetMist = nearMist*(0.38+0.42*warm)*(1.0-0.18*cool);
    float rainTone = smoothstep(0.06,0.30,1.0-lum)*smoothstep(0.02,0.25,abs(cool-warm));
    float rainMist = 0.0;
    #ifdef NL_RAIN_MIST_OPACITY
      rainMist = NL_RAIN_MIST_OPACITY*rainTone;
    #endif
    float shaped = base+(1.0-base)*(0.24*mist+0.32*sunsetMist+0.38*rainMist*distNorm);
    shaped = smoothstep(0.0,1.0,shaped);
    return NL_FOG*clamp(shaped,0.0,1.0);
  #else
    return 0.0;
  #endif
}

float nlRenderGodRayIntensity(vec3 cPos, vec3 worldPos, float t, vec2 uv1, float relativeDist, vec3 FOG_COLOR) {
  vec3 offset = cPos-16.0*fract(worldPos*0.0625);
  offset = abs(2.0*fract(offset*0.0625)-1.0);
  offset = offset*offset*(3.0-2.0*offset);
  vec3 nrmof = normalize(worldPos+vec3(0.0001,0.0001,0.0001));
  float u = nrmof.z/max(length(nrmof.zy),0.0001);
  float diff = dot(offset,vec3(0.1,0.2,1.0))+0.055*t;
  float mask = nrmof.x*nrmof.x;
  float p0 = sin(5.0*u+1.15*diff);
  float p1 = sin(9.0*u-0.65*diff+1.3*p0);
  float p2 = sin(15.0*u+0.4*diff);
  float vol = 0.55*p0*p0+0.30*p1*p1+0.15*p2*p2;
  vol *= (0.28+0.72*mask)*uv1.y;
  vol *= smoothstep(0.0,1.0,relativeDist)*smoothstep(1.0,0.12,relativeDist);
  float warm = nlFogWarmMask(FOG_COLOR);
  float cool = nlFogCoolMask(FOG_COLOR);
  float dawn = smoothstep(0.0,0.35,warm);
  float strength = 0.10+0.72*dawn+0.12*cool;
  float haze = smoothstep(0.18,0.95,dot(FOG_COLOR,vec3_splat(0.333333)));
  vol *= strength*(0.55+0.45*haze)*(1.0-0.65*cool*step(0.15,warm));
  vol = smoothstep(0.24,0.92,vol);
  return clamp(vol,0.0,1.0);
}

#endif
