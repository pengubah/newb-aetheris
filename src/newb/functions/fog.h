#ifndef FOG_H
#define FOG_H

float nlRenderFogFade(float relativeDist,vec3 FOG_COLOR,vec2 FOG_CONTROL) {
  #ifdef NL_FOG
    float range=smoothstep(FOG_CONTROL.x,FOG_CONTROL.y,relativeDist);
    float colorDensity=NL_MIST_DENSITY*(15.0-13.0*clamp(FOG_COLOR.g,0.0,1.0));
    float mist=1.0-exp(-relativeDist*relativeDist*colorDensity*0.0025);
    float horizonBias=1.0-abs(FOG_COLOR.r-FOG_COLOR.b);
    mist*=0.68+0.32*clamp(horizonBias,0.0,1.0);
    float rainBoost=1.0+0.55*NL_RAIN_MIST_OPACITY;
    mist=1.0-pow(max(1.0-mist,0.0),rainBoost);
    float dawnAtmosphere=1.0;
    float configWarm=clamp(NL_DAWN_SUNLIGHT_COL.r-NL_DAWN_SUNLIGHT_COL.b,0.0,1.0);
  float warm=clamp(FOG_COLOR.r-FOG_COLOR.b,0.0,1.0)+0.35*configWarm;
    float cool=clamp(FOG_COLOR.b-FOG_COLOR.r,0.0,1.0);
    dawnAtmosphere+=0.22*warm+0.08*cool;
    mist*=dawnAtmosphere;
    float fade=mix(range,mix(range,1.0,mist),NL_MIST_DENSITY*0.90);
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

  float configWarm=clamp(NL_DAWN_SUNLIGHT_COL.r-NL_DAWN_SUNLIGHT_COL.b,0.0,1.0);
  float warm=clamp(FOG_COLOR.r-FOG_COLOR.b,0.0,1.0)+0.35*configWarm;
  float dawnWarm=pow(warm,1.25);
  float haze=clamp(1.0-0.55*FOG_COLOR.g,0.18,1.0);
  vol*=clamp(2.6*(warm+0.25*dawnWarm),0.0,1.0);
  vol*=haze;

  // Two smooth layers avoid the blocky/stepped godray appearance.
  float broad=smoothstep(0.0,0.18,vol);
  float detail=smoothstep(0.0,0.055,vol*1.35+0.02*sin(t+diff));
  float result=mix(broad,0.65*broad+0.35*detail,0.55);
  result*=1.0-0.55*NL_RAIN_MIST_OPACITY;
  return NL_GODRAY*result;
}

#endif
