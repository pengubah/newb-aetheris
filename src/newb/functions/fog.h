#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist, vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float fade = smoothstep(FOG_CONTROL.x,FOG_CONTROL.y,relativeDist);

    // cinematic distance fog
    float density = NL_MIST_DENSITY*(18.0-16.5*FOG_COLOR.g);
    float mist = 1.0-exp(-relativeDist*relativeDist*density*0.055);
    mist = smoothstep(0.0,0.92,mist);

    // soften fog near the camera
    float nearFade = smoothstep(0.0,0.16,relativeDist);
    mist *= nearFade;

    // combine distance fog and atmospheric mist
    fade = mix(fade,fade+(1.0-fade)*mist,0.72);

    // subtle atmospheric depth variation
    float atmosphere = 1.0-exp(-relativeDist*0.018);
    atmosphere *= NL_MIST_DENSITY*0.18;
    fade += (1.0-fade)*atmosphere;

    return NL_FOG*clamp(fade,0.0,1.0);
  #else
    return 0.0;
  #endif
}

float nlRenderGodRayIntensity(vec3 cPos, vec3 worldPos, float t, vec2 uv1, float relativeDist, vec3 FOG_COLOR) {
  // offset wPos (only works upto 16 blocks)
  vec3 offset = cPos-16.0*fract(worldPos*0.0625);
  offset = abs(2.0*fract(offset*0.0625)-1.0);
  offset = offset*offset*(3.0-2.0*offset);

  // normalized world direction
  vec3 nrmof = normalize(worldPos);

  // directional coordinates
  float u = nrmof.z/max(length(nrmof.zy),0.0001);
  float diff = dot(offset,vec3(0.1,0.2,1.0))+0.07*t;

  // horizon/sun direction mask
  float mask = nrmof.x*nrmof.x;
  float horizonMask = 1.0-mask;

  // layered volumetric structure
  float ray1 = sin(7.0*u+1.5*diff);
  float ray2 = sin(3.0*u+diff);
  float ray3 = sin(11.0*u-0.65*diff);

  ray1 *= ray1;
  ray2 = ray2*ray2;
  ray3 = ray3*ray3;

  float vol = ray1*0.48+ray2*0.34+ray3*0.18;
  vol *= mask;
  vol *= uv1.y;
  vol *= 0.55+0.45*horizonMask;

  // distance scattering
  float distanceFade = smoothstep(0.02,0.85,relativeDist);
  distanceFade *= distanceFade;
  vol *= distanceFade;

  // dawn/dusk color separation
  float dawnColor = clamp(FOG_COLOR.r-FOG_COLOR.b,0.0,1.0);
  dawnColor = smoothstep(0.025,0.22,dawnColor);

  // warm sunrise/sunset concentration
  float warmGlow = clamp(FOG_COLOR.r-0.35*FOG_COLOR.b,0.0,1.0);
  warmGlow = smoothstep(0.15,0.85,warmGlow);

  vol *= dawnColor*(0.72+0.28*warmGlow);

  // soft threshold
  vol = smoothstep(0.015,0.22,vol);

  // cinematic attenuation
  vol *= 0.72+0.28*uv1.y;
  vol *= 0.82+0.18*smoothstep(0.0,1.0,relativeDist);

  return clamp(vol,0.0,1.0);
}

#endif