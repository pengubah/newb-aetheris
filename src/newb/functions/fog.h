#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist, vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float fade = smoothstep(FOG_CONTROL.x, FOG_CONTROL.y, relativeDist);

    // Dawn / dusk color influence. Use the configured Dawn palette as the
    // warm bias, while retaining FOG_COLOR as the base atmospheric color.
    float dawn = clamp(3.2*(FOG_COLOR.r - FOG_COLOR.b), 0.0, 1.0);
    dawn *= dawn;

    vec3 dawnPalette = mix(NL_DAWN_EDGE_COL, NL_DAWN_HORIZON_COL, 0.35);
    dawnPalette = mix(dawnPalette, NL_DAWN_SUNLIGHT_COL, 0.25);
    vec3 dawnFog = mix(FOG_COLOR, FOG_COLOR*dawnPalette*1.25, dawn*0.34);

    // Broad soft atmospheric haze, strongest close to the horizon.
    float density = NL_MIST_DENSITY*(18.0 - 14.0*dawn);
    float mist = 1.0-exp(-relativeDist*relativeDist*density);

    fade += (1.0-fade)*mist*(0.24 + 0.18*dawn);

    return NL_FOG * fade;
  #else
    return 0.0;
  #endif
}

float nlRenderGodRayIntensity(vec3 cPos, vec3 worldPos, float t, vec2 uv1, float relativeDist, vec3 FOG_COLOR) {
  // Offset wPos
  vec3 offset = cPos - 16.0*fract(worldPos*0.0625);
  offset = abs(2.0*fract(offset*0.0625)-1.0);
  offset = offset*offset*(3.0-2.0*offset);

  vec3 nrmof = normalize(worldPos);

  float u = nrmof.z/length(nrmof.zy);

  float diff = dot(offset, vec3(0.1,0.2,1.0)) + 0.07*t;

  float mask = nrmof.x*nrmof.x;

  // Soft volumetric pattern
  float wave1 = sin(5.0*u + 1.15*diff);
  float wave2 = sin(2.7*u - 0.75*diff);
  float wave3 = sin(8.0*u + 0.45*diff);

  float vol = wave1*wave1;
  vol *= 0.55 + 0.45*wave2*wave2;
  vol *= 0.72 + 0.28*wave3*wave3;

  // Directional mask
  float rayMask = mask*(1.0-mask);
  rayMask = rayMask*rayMask*3.5;

  vol *= rayMask;
  vol *= uv1.y;

  // Distance attenuation
  float distFade = clamp(relativeDist*0.055, 0.0, 1.0);
  vol *= distFade;

  // Dawn / dusk influence. The strength follows the configured warm Dawn
  // palette without introducing a separate hard-coded sky color.
  float dawn = clamp(3.0*(FOG_COLOR.r - FOG_COLOR.b), 0.0, 1.0);
  dawn *= dawn;

  vol *= dawn;
  vol *= 0.78 + 0.22*dawn;

  // Soft, wide rays like the reference image rather than sharp beams.
  vol = smoothstep(0.012, 0.38, vol);
  vol *= 0.64 + 0.36*(1.0-mask);

  return vol*NL_GODRAY;
}

#endif
