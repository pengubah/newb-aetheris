#ifndef SKY_H
#define SKY_H

#include "detection.h"
#include "noise.h"

struct nl_skycolor {
  vec3 zenith;
  vec3 horizon;
  vec3 horizonEdge;
};

vec3 spectrum(float x) {
  vec3 s=vec3(x-0.5,x,x+0.5);
  s=smoothstep(1.0,0.0,abs(s));
  return s*s;
}

vec3 getUnderwaterCol(vec3 FOG_COLOR) {
  vec3 f=max(FOG_COLOR,vec3_splat(0.0));
  return 1.65*NL_UNDERWATER_TINT*f*f+0.12*f;
}

vec3 getEndZenithCol() { return NL_END_ZENITH_COL; }
vec3 getEndHorizonCol() { return NL_END_HORIZON_COL; }

nl_skycolor nlEndSkyColors(nl_environment env) {
  nl_skycolor s;
  float voidStrength=0.75+0.25*(1.0-env.rainFactor);
  s.zenith=NL_END_ZENITH_COL*voidStrength;
  s.horizon=NL_END_HORIZON_COL*(0.88+0.12*voidStrength);
  s.horizonEdge=mix(s.horizon,0.55*NL_END_HORIZON_COL+0.18*NL_END_ZENITH_COL,0.35);
  return s;
}

nl_skycolor nlOverworldSkyColors(nl_environment env) {
  nl_skycolor s;
  float df=env.dayFactor;
  float night=smoothstep(0.08,-0.20,df);
  float dawn=pow(saturate(1.0-df*df),2.2);
  float lowSun=smoothPulse(df,-0.38,0.42);
  float nightLift=1.0+0.35*night;

  s.zenith=mix(NL_DAY_ZENITH_COL,NL_NIGHT_ZENITH_COL*nightLift,night);
  s.horizon=mix(NL_DAY_HORIZON_COL,NL_NIGHT_HORIZON_COL*nightLift,night);
  s.horizonEdge=mix(NL_DAY_EDGE_COL,NL_NIGHT_EDGE_COL*nightLift,night);

  vec3 dawnZen=NL_DAWN_ZENITH_COL*mix(0.72,1.0,lowSun);
  vec3 dawnHor=NL_DAWN_HORIZON_COL*(0.78+0.22*lowSun);
  vec3 dawnEdge=NL_DAWN_EDGE_COL*(0.80+0.20*lowSun);
  s.zenith=mix(s.zenith,dawnZen,dawn*0.78);
  s.horizon=mix(s.horizon,dawnHor,dawn);
  s.horizonEdge=mix(s.horizonEdge,dawnEdge,dawn);

  float zh=max(luminance(s.zenith),0.001);
  float hh=max(luminance(s.horizon),0.001);
  float rainMix=env.rainFactor*NL_SKY_RAIN_MIX_FACTOR;
  s.zenith=mix(s.zenith,NL_RAIN_ZENITH_COL*zh*1.35,rainMix);
  s.horizon=mix(s.horizon,NL_RAIN_HORIZON_COL*hh*1.15,rainMix);
  s.horizonEdge=mix(s.horizonEdge,mix(s.horizon,NL_RAIN_HORIZON_COL*hh,0.35),env.rainFactor);

  if(env.underwater) {
    vec3 uw=getUnderwaterCol(env.fogCol);
    float depth=saturate(0.45+0.55*env.fogCol.b);
    s.zenith=mix(uw*1.6,uw*0.55,depth);
    s.horizon=mix(uw*1.25,uw*0.75,depth);
    s.horizonEdge=s.horizon*1.04;
  }
  return s;
}

nl_skycolor nlSkyColors(nl_environment env) {
  if(env.end) return nlEndSkyColors(env);
  return nlOverworldSkyColors(env);
}

vec3 renderOverworldSky(nl_skycolor skyCol, nl_environment env, vec3 viewDir, bool isSkyPlane) {
  float vy=viewDir.y;
  float av=abs(vy);
  float horizonMask=1.0-smoothstep(0.0,0.95,av);
  float mask=0.5+0.5*vy/(0.38+av);

  vec2 sunMoon=vec2(dot(env.sunDir,viewDir),dot(env.moonDir,viewDir));
  vec2 g=saturate3(vec3(0.5-0.5*sunMoon.x,0.5-0.5*sunMoon.y,0.0)).xy;
  vec2 soft=sqrt(g);
  vec2 source=mix(soft,g,env.rainFactor);
  source=source*source;
  float sunDisc=pow(saturate(1.0-source.x),18.0);
  float moonDisc=pow(saturate(1.0-source.y),14.0);
  float sourceMask=(sunDisc*(1.0-env.rainFactor)+0.35*moonDisc*env.dayFactor*env.dayFactor);

  float lower=1.0-vy*vy;
  float gradient=pow(saturate(lower),mix(0.9,3.6,abs(vy)));
  gradient=mix(gradient,1.0,sourceMask);
  float vertical=smoothstep(-1.0,1.0,vy);
  vec3 lowerSky=mix(skyCol.horizon,skyCol.horizonEdge,pow(horizonMask,1.35));
  vec3 sky=mix(lowerSky,skyCol.zenith,pow(vertical,0.72));
  sky=mix(sky,skyCol.horizonEdge,0.20*horizonMask*(1.0-gradient));

  float dawn=pow(saturate(1.0-env.dayFactor*env.dayFactor),2.4);
  float warmHorizon=smoothstep(0.02,0.75,horizonMask)*dawn*(1.0-env.rainFactor);
  sky+=0.22*NL_DAWN_EDGE_COL*warmHorizon;

  float voidMask=smoothstep(0.15,0.92,-vy);
  float voidDim=mix(1.0,NL_SKY_VOID_DARKNESS,voidMask*NL_SKY_VOID_FACTOR);
  sky*=voidDim;
  sky*=0.72+0.28*gradient;
  sky*=1.0+sourceMask*(6.0+5.0*sourceMask)*(1.0-env.rainFactor);

  #ifdef NL_RAINBOW
    float rainbowFade=smoothstep(0.05,0.55,vy);
    rainbowFade*=rainbowFade;
    rainbowFade*=mix(NL_RAINBOW_CLEAR,NL_RAINBOW_RAIN,env.rainFactor);
    rainbowFade*=smoothstep(-0.25,0.55,env.dayFactor);
    sky+=spectrum(22.0*(0.82-sunMoon.x))*rainbowFade*skyCol.horizon*0.75;
  #endif
  return max(sky,vec3_splat(0.0));
}


vec4 renderBlackhole(vec3 vdir, float t) {
  t *= NL_BH_SPEED;

  float r = 2.15;
  vec3 vr = vdir;

  float cr = cos(r);
  float sr = sin(r);

  vec2 rot;
  rot.x = cr*vr.x-sr*vr.y;
  rot.y = sr*vr.x+cr*vr.y;
  vr.xy = rot;

  vec3 vd = vr-vec3(0.0,-0.92,0.0);

  // blackhole distance
  float d = length(vd);

  float nl = sin(16.5*vd.x+t)*sin(16.5*vd.y-t)*sin(16.5*vd.z+t);

  float df = sin(3.0*vd.x-4.0*d+24.0*pow(1.4-d,4.0)+t);

  float d0 = (0.64-d)/0.6;
  float dm0 = 1.0-max(d0,0.0);

  float gl = 1.0-clamp(-0.3*d0,0.0,1.0);

  float gla = pow(1.0-min(abs(d0),1.0),14.0);

  float gl8 = pow(gl,12.0);

  float hole = 0.86*pow(dm0,38.0)+0.14*pow(dm0,6.0);

  float bh = (gla+0.8*gl8+0.2*gl8*gl8)*hole;

  df *= 0.9+0.1*sin(8.0*vd.z+d+4.0*t - 4.0*df);

  bh *= 1.0+pow(df,4.0)*hole*max(1.0-bh,0.0);

  vec3 col = bh*4.35*mix(NL_BH_COL_LOW,NL_BH_COL_HIGH,min(bh,1.0));

  return vec4(col,hole);
}

vec3 renderEndSky(vec3 horizonCol, vec3 zenithCol, vec3 viewDir, float t) {
  t*=0.06;
  float a=atan2(viewDir.x,viewDir.z);
  float band=0.5+0.5*sin(2.4*a+t+2.0*sin(0.35*a+viewDir.y*3.0));
  float band2=0.5+0.5*sin(5.1*a-0.6*t+3.0*band);
  float filament=0.5+0.5*sin(13.0*a+2.2*t+4.0*band2);
  float vertical=pow(saturate(0.5+0.5*viewDir.y),0.7);
  float voidEdge=pow(saturate(1.0-abs(viewDir.y)),0.45);
  float nebula=saturate(0.58*band+0.30*band2+0.12*filament);
  vec3 base=mix(horizonCol*0.68,zenithCol,vertical);
  vec3 violet=vec3(0.32,0.035,0.68);
  vec3 magenta=vec3(0.82,0.05,0.42);
  vec3 cyan=vec3(0.05,0.22,0.50);
  vec3 neb=mix(violet,magenta,band2);
  neb=mix(neb,cyan,0.18*filament);
  base=mix(base,base*0.35+neb*0.55,nebula*voidEdge);
  float streak=smoothstep(0.18,0.82,filament)*pow(voidEdge,1.4);
  base+=streak*(0.11*magenta+0.035*cyan);
  float rim=pow(saturate(1.0-abs(viewDir.y)),4.0);
  base+=rim*rim*vec3(0.22,0.015,0.12);

  #ifdef NL_BLACKHOLE
    vec4 bh = renderBlackhole(viewDir, t);
    base *= bh.a;
    base += bh.rgb;
  #endif

  return max(base,vec3_splat(0.0));
}

vec3 nlRenderSky(nl_skycolor skycol, nl_environment env, vec3 viewDir, float t, bool isSkyPlane) {
  viewDir.y=-viewDir.y;
  if(env.end) return renderEndSky(skycol.horizon,skycol.zenith,viewDir,t);
  return renderOverworldSky(skycol,env,viewDir,isSkyPlane);
}

vec3 nlRenderShootingStar(vec3 viewDir, vec3 FOG_COLOR, float t) {
  float period=max(NL_SHOOTING_STAR_PERIOD,0.1);
  float cycle=NL_SHOOTING_STAR_DELAY+period;
  float h=t/max(cycle,0.1);
  float h0=floor(h);
  float local=cycle*fract(h);
  float p=saturate(local/period);
  float p2=p*p*(3.0-2.0*p);
  float fade=(1.0-p2)*(1.0-p2);
  float r=fract(sin(h0*12.9898)*43758.5453);
  float ang=6.2831*r;
  float c=cos(ang),s=sin(ang);
  vec2 uv=viewDir.xz*(6.0+4.0*r);
  uv=vec2(c*uv.x+s*uv.y,-s*uv.x+c*uv.y);
  uv.x+=fade-p-2.0*r-3.5;
  uv.y+=viewDir.y*3.0;
  float source=1.0-saturate(abs(uv.x-0.95)*22.0);
  float line=1.0-saturate(abs(uv.y)*9.0);
  float tail=smoothstep(-1.0+1.8*fade,0.98-p,uv.x);
  float head=smoothstep(1.0,0.98-p2,uv.x);
  float star=line*line*line*tail*head*(1.0-p)*0.65;
  star*=0.6+12.0*source*source;
  star*=1.0-saturate(luminance(FOG_COLOR)*1.4);
  return NL_SHOOTING_STAR*star*vec3(0.72,0.86,1.0);
}

vec3 nlRenderGalaxy(vec3 vdir, vec3 fogColor, nl_environment env, float t) {
  if(env.underwater) return vec3_splat(0.0);
  t*=NL_GALAXY_SPEED;
  float c=cos(0.18*t),s=sin(0.18*t);
  vdir.xy=mul(mat2(c,s,-s,c),vdir.xy);
  float n0=0.5+0.5*sin(4.0*vdir.x+0.4*t)*sin(4.0*vdir.y-0.3*t)*sin(4.0*vdir.z+0.2*t);
  float n1=noise3D(13.0*vdir+sin(0.7*t+1.3));
  float n2=noise3D(42.0*vdir+0.8*n1+sin(0.45*t));
  float n3=noise3D(160.0*vdir-4.0*sin(0.3*t));
  float star=smoothstep(0.28,0.78,n3+0.18*n2);
  float arm=pow(saturate(1.0-abs(vdir.x+0.15*vdir.y+0.12*sin(8.0*vdir.z))),3.0);
  star*=star*(0.45+0.55*arm);
  vec3 starCol=0.78+0.22*sin(vec3(8.0,6.0,10.0)*(n1+n2)+vec3(0.0,0.4,0.82));
  float neb=pow(saturate(1.0-(vdir.x*vdir.x+0.04*n1+0.16*n0)),3.0);
  vec3 nebCol=normalize(vec3(n0+0.1,0.3+cos(2.0*vdir.y),0.25+sin(vdir.x+n0)));
  vec3 stars=starCol*star*(1.0+4.0*star)+neb*(0.012+0.25*neb)*mix(vec3_splat(0.45),nebCol*nebCol,NL_GALAXY_VIBRANCE);
  float night=smoothstep(0.35,-0.15,env.dayFactor);
  stars*=mix(NL_GALAXY_DAY_VISIBILITY,1.0,night);
  stars*=1.0-0.82*env.rainFactor;
  stars*=1.0-saturate(luminance(fogColor)*0.75);
  return stars;
}

#endif
