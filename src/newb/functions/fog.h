#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist,vec3 FOG_COLOR,vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float range=smoothstep(FOG_CONTROL.x,FOG_CONTROL.y,relativeDist);
    float colorDensity=NL_MIST_DENSITY*(15.0-13.0*clamp(FOG_COLOR.g,0.0,1.0));

    // Preserve the configured NL_MIST_DENSITY while adding a restrained atmospheric veil.
    float mist=1.0-exp(-relativeDist*relativeDist*colorDensity*0.0027);
    float horizonBias=1.0-abs(FOG_COLOR.r-FOG_COLOR.b);
    mist*=0.70+0.30*clamp(horizonBias,0.0,1.0);

    // Warm fog naturally becomes stronger when the configured dawn palette is active.
    float warmFog=smoothstep(0.02,0.22,FOG_COLOR.r-FOG_COLOR.b);
    mist*=1.0+0.14*warmFog;

    float fade=mix(range,mix(range,1.0,mist),NL_MIST_DENSITY*0.88);
    return clamp(NL_FOG*fade,0.0,1.0);
  #else
    return 0.0;
  #endif
}

float nlRenderGodRayIntensity(vec3 cPos,vec3 worldPos,float t,vec2 uv1,float relativeDist,vec3 FOG_COLOR) {
  vec3 offset=cPos-16.0*fract(worldPos*0.0625);
  offset=abs(2.0*fract(offset*0.0625)-1.0);
  offset=offset*offset*(3.0-2.0*offset);

  vec3 nrmof=normalize(worldPos);
  float u=nrmof.z/length(nrmof.zy+vec2_splat(0.0001));
  float diff=dot(offset,vec3(0.1,0.2,1.0))+0.07*t;
  float mask=nrmof.x*nrmof.x;

  float vol=sin(7.0*u+1.5*diff)*sin(3.0*u+diff);
  float fine=sin(13.0*u-0.8*diff);
  vol=(vol+0.35*fine)*(vol+0.35*fine)*mask*uv1.y*(1.0-mask*mask);
  vol*=relativeDist*relativeDist;

  float warmFog=smoothstep(0.02,0.22,FOG_COLOR.r-FOG_COLOR.b);
  float dawnFog=clamp(3.0*(FOG_COLOR.r-FOG_COLOR.b),0.0,1.0);
  float blueFade=1.0-smoothstep(0.18,0.55,FOG_COLOR.b);

  vol*=dawnFog;
  vol*=0.72+0.48*warmFog;
  vol*=0.78+0.22*blueFade;

  // Slightly broaden the shafts so they blend into fog instead of looking striped.
  return NL_GODRAY*smoothstep(0.0,0.105,vol);
}

#endif
