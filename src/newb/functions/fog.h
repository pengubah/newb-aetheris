#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist, vec3 FOG_COLOR, vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float base=smoothstep(FOG_CONTROL.x,FOG_CONTROL.y,relativeDist);
    float dist2=relativeDist*relativeDist;
    float lum=dot(saturate3(FOG_COLOR),vec3(0.2126,0.7152,0.0722));
    float colorDensity=mix(1.25,0.55,lum);
    float mist=1.0-exp(-dist2*NL_MIST_DENSITY*0.0025*colorDensity);
    float rainMist=saturate(0.5*(FOG_COLOR.r+FOG_COLOR.g+FOG_COLOR.b));
    float cloudy=NL_CLOUDY_FOG*(1.0-smoothstep(0.15,0.65,lum));
    float result=max(base,mix(mist,1.0-mix(1.0,mist,0.55),cloudy));

    #ifdef NL_RAIN_MIST_OPACITY
      result=mix(result,saturate(result+0.12*rainMist),NL_RAIN_MIST_OPACITY);
    #endif

    return NL_FOG*saturate(result);
  #else
    return 0.0;
  #endif
}

float nlRenderGodRayIntensity(vec3 cPos, vec3 worldPos, float t, vec2 uv1, float relativeDist, vec3 FOG_COLOR) {
  vec3 offset=cPos-16.0*fract(worldPos*0.0625);
  offset=abs(2.0*fract(offset*0.0625)-1.0);
  offset=offset*offset*(3.0-2.0*offset);
  vec3 nrmof=safeNormalize(worldPos);
  float denom=max(length(nrmof.zy),0.001);
  float u=nrmof.z/denom;
  float diff=dot(offset,vec3(0.1,0.2,1.0))+0.07*t;
  float mask=nrmof.x*nrmof.x;
  float vol=sin(7.0*u+1.5*diff)*sin(3.0*u+diff);
  vol*=vol*mask*uv1.y*(1.0-mask*mask);
  float distanceFade=smoothstep(0.04,0.9,relativeDist*relativeDist);
  vol*=distanceFade;
  float warm=saturate(2.7*(FOG_COLOR.r-FOG_COLOR.b)+0.6*(FOG_COLOR.r-FOG_COLOR.g));
  vol*=warm;
  return smoothstep(0.015,0.13,vol)*NL_GODRAY;
}

#endif
