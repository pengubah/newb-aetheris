#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist, vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float fade = smoothstep(FOG_CONTROL.x, FOG_CONTROL.y, relativeDist);

    // misty effect
    float dawnHaze = clamp(3.0*(FOG_COLOR.r-FOG_COLOR.b),0.0,1.0);
    float density = NL_MIST_DENSITY*(19.0-18.0*FOG_COLOR.g)*(1.0+0.65*dawnHaze);
    float haze = 0.3-0.3*exp(-relativeDist*relativeDist*density);
    fade += (1.0-fade)*haze;
    fade += (1.0-fade)*haze*0.28*dawnHaze;

    return NL_FOG * fade;
  #else
    return 0.0;
  #endif
}

float nlRenderGodRayIntensity(vec3 cPos, vec3 worldPos, float t, vec2 uv1, float relativeDist, vec3 FOG_COLOR, float dayFactor, vec3 sunDir) {
  // offset wPos (only works upto 16 blocks)
  vec3 offset = cPos - 16.0*fract(worldPos*0.0625);
  offset = abs(2.0*fract(offset*0.0625)-1.0);
  offset = offset*offset*(3.0-2.0*offset);
  //offset = 0.5 + 0.5*cos(offset*0.392699082);

  //vec3 ofPos = wPos+offset;
  vec3 nrmof = normalize(worldPos);

  float sunView = smoothstep(0.92,1.0,dot(nrmof,normalize(sunDir)));

  float u = nrmof.z/length(nrmof.zy);
  float diff = dot(offset,vec3(0.1,0.2,1.0)) + 0.07*t;
  float mask = nrmof.x*nrmof.x;

  float vol = sin(7.0*u + 1.5*diff)*sin(3.0*u + diff);
  vol *= vol*mask*uv1.y*(1.0-mask*mask);
  vol *= relativeDist*relativeDist;

  // dawn/dusk mask
  float dawnMask = clamp(3.0*(FOG_COLOR.r-FOG_COLOR.b),0.0,1.0);

  // day musk
  float dayMask = smoothstep(0.08,0.35,dayFactor)*sunView;

  vol *= max(dawnMask,dayMask);
  
  vol = smoothstep(0.0, 0.1, vol);
  return vol;
}

#endif
