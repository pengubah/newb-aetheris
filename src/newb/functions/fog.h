#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist,vec3 FOG_COLOR,vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float range=smoothstep(FOG_CONTROL.x,FOG_CONTROL.y,relativeDist);
    float colorDensity=NL_MIST_DENSITY*(15.0-13.0*clamp(FOG_COLOR.g,0.0,1.0));
    float mist=1.0-exp(-relativeDist*relativeDist*colorDensity*0.0025);
    float horizonBias=1.0-abs(FOG_COLOR.r-FOG_COLOR.b);
    mist*=0.72+0.28*clamp(horizonBias,0.0,1.0);
    float fade=mix(range,mix(range,1.0,mist),NL_MIST_DENSITY*0.85);
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
  vol*=clamp(3.0*(FOG_COLOR.r-FOG_COLOR.b),0.0,1.0);
  return NL_GODRAY*smoothstep(0.0,0.12,vol);
}

#endif
