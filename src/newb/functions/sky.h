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
  vec3 s = vec3(x-0.5,x,x+0.5);
  s = smoothstep(1.0,0.0,abs(s));
  return s*s;
}

vec3 getUnderwaterCol(vec3 FOG_COLOR) {
  return 2.0*NL_UNDERWATER_TINT*FOG_COLOR*FOG_COLOR;
}

vec3 getEndZenithCol() { return NL_END_ZENITH_COL; }
vec3 getEndHorizonCol() { return NL_END_HORIZON_COL; }

nl_skycolor nlEndSkyColors(nl_environment env) {
  nl_skycolor s;
  s.zenith = NL_END_ZENITH_COL;
  s.horizon = NL_END_HORIZON_COL;
  s.horizonEdge = mix(s.horizon,s.zenith,0.35);
  return s;
}

nl_skycolor nlOverworldSkyColors(nl_environment env) {
  nl_skycolor s;
  float d = clamp(env.dayFactor,-1.0,1.0);
  float day = smoothstep(-0.05,0.18,d);
  float night = smoothstep(0.08,-0.22,d);
  float dawn = pow(1.0-d*d,3.0);

  s.zenith = mix(NL_NIGHT_ZENITH_COL,NL_DAY_ZENITH_COL,day);
  s.horizon = mix(NL_NIGHT_HORIZON_COL,NL_DAY_HORIZON_COL,day);
  s.horizonEdge = mix(NL_NIGHT_EDGE_COL,NL_DAY_EDGE_COL,day);

  vec3 dawnZenith = NL_DAWN_ZENITH_COL;
  vec3 dawnHorizon = NL_DAWN_HORIZON_COL;
  vec3 dawnEdge = NL_DAWN_EDGE_COL;
  float dawnMask = dawn*(1.0-0.18*night);
  // Dawn colors come directly from NL_CONFIG_H; keep warmth concentrated near horizon.
  s.zenith = mix(s.zenith,dawnZenith,dawnMask*0.62);
  s.horizon = mix(s.horizon,dawnHorizon,dawnMask*0.90);
  s.horizonEdge = mix(s.horizonEdge,dawnEdge,dawnMask*1.04);

  float rainMix = env.rainFactor*NL_SKY_RAIN_MIX_FACTOR;
  float zh = max(dot(s.zenith,vec3_splat(0.333)),0.0);
  float hh = max(dot(s.horizon,vec3_splat(0.333)),0.0);
  s.zenith = mix(s.zenith,NL_RAIN_ZENITH_COL*zh*1.35,rainMix);
  s.horizon = mix(s.horizon,NL_RAIN_HORIZON_COL*hh*1.25,rainMix);
  s.horizonEdge = mix(s.horizonEdge,s.horizon,env.rainFactor*0.82);

  if (env.underwater) {
    vec3 waterFog = max(env.fogCol,vec3_splat(0.0));
    waterFog *= waterFog;
    waterFog *= NL_UNDERWATER_TINT;
    s.zenith = mix(s.zenith,waterFog*1.6,0.82);
    s.horizon = mix(s.horizon,waterFog*2.2,0.90);
    s.horizonEdge = s.horizon;
  }
  return s;
}

nl_skycolor nlSkyColors(nl_environment env) {
  if (env.end) return nlEndSkyColors(env);
  return nlOverworldSkyColors(env);
}

vec3 renderOverworldSky(nl_skycolor skyCol,nl_environment env,vec3 viewDir,bool isSkyPlane) {
  float y = clamp(viewDir.y,-1.0,1.0);
  float av = abs(y);
  float mask = 0.5+0.5*y/(0.34+av);

  vec2 sunMoon = clamp(0.5-0.5*vec2(dot(env.sunDir,viewDir),dot(env.moonDir,viewDir)),0.0,1.0);
  vec2 glow2 = 1.0-mix(sqrt(sunMoon),sunMoon,env.rainFactor);
  vec2 glow4 = glow2*glow2;
  vec2 glow8 = glow4*glow4;
  float source = (glow8.x+glow8.y)*mask*(1.0-0.94*env.rainFactor);

  float horizon = 1.0-y*y;
  float h2 = horizon*horizon;
  float voidMix = step(y,0.0)*NL_SKY_VOID_FACTOR;
  h2 = mix(h2,mix(1.0,h2*h2,voidMix),step(y,0.0));
  h2 = mix(h2,1.0,source);
  float h4 = h2*h2;

  float g1 = h4*h4;
  float g2 = 0.75*g1+0.25*h2;
  g1 *= g1;
  g1 = mix(g1*g1,1.0,source);
  g2 = mix(g2,1.0,source);

  float dawn = pow(max(1.0-env.dayFactor*env.dayFactor,0.0),3.0);
  float df = mix(1.0,glow4.x,dawn*dawn);
  vec3 sky = mix(skyCol.horizon,skyCol.horizonEdge,g1*df*df);
  sky = mix(skyCol.zenith,sky,g2*df);

  float horizonLift = 0.74+0.26*g2;
  sky *= horizonLift;
  sky *= (1.0+(1.8*source+6.0*source*source)*mask)*mix(1.0,mask,NL_SKY_VOID_DARKNESS);

  if (!isSkyPlane) {
    float s = pow(max((source-0.18)/0.82,0.0),2.0);
    sky *= 1.0+22.0*s*(1.0-env.rainFactor);
  }

  // Smooth radial sun/moon glow. No extra sky color config is introduced here.
  float sunDot=max(dot(env.sunDir,viewDir),0.0);
  float moonDot=max(dot(env.moonDir,viewDir),0.0);
  float sunHalo=exp2(-14.0*(1.0-sunDot));
  float moonHalo=exp2(-17.0*(1.0-moonDot));
  float sunDisc=smoothstep(0.9990,0.99994,sunDot);
  float moonDisc=smoothstep(0.9992,0.99996,moonDot);
  float dawnHalo=smoothstep(0.72,0.98,1.0-abs(env.dayFactor));
  vec3 sunTint=mix(NL_DAWN_SUNLIGHT_COL,NL_NOON_SUNLIGHT_COL,smoothstep(-0.08,0.58,env.dayFactor));
  float sunVis=1.0-0.82*env.rainFactor;
  float moonVis=(1.0-env.rainFactor)*step(env.dayFactor,0.0);
  sky += sunHalo*(0.12+0.09*dawnHalo)*sunTint*sunVis;
  sky += sunDisc*1.65*sunTint*sunVis;
  sky += moonHalo*0.105*NL_NIGHT_MOONLIGHT_COL*moonVis;
  sky += moonDisc*0.18*NL_NIGHT_MOONLIGHT_COL*moonVis;

  #ifdef NL_RAINBOW
    float rainbowFade = pow(max(0.5+0.5*y,0.0),2.0);
    rainbowFade *= mix(NL_RAINBOW_CLEAR,NL_RAINBOW_RAIN,env.rainFactor);
    rainbowFade *= 0.5+0.5*max(env.dayFactor,0.0);
    float rb = 24.2*(0.85-sunMoon.x);
    sky += spectrum(rb)*rainbowFade*skyCol.horizon*1.35;
  #endif

  return sky;
}

vec4 renderBlackhole(vec3 vdir,float t) {
  t *= NL_BH_SPEED;
  float r = 2.4;
  vec3 vr = vdir;
  float cr = cos(r);
  float sr = sin(r);
  vec2 rot;
  rot.x = cr*vr.x-sr*vr.y;
  rot.y = sr*vr.x+cr*vr.y;
  vr.xy = rot;
  vec3 vd = vr-vec3(0.0,-1.0,0.0);
  float d = length(vd);
  float nl = sin(15.0*vd.x+t)*sin(15.0*vd.y-t)*sin(15.0*vd.z+t);
  float df = sin(3.0*vd.x-4.0*d+24.0*pow(1.4-d,4.0)+t);
  float bhRadius=0.6*max(NL_BH_DIST,0.1);
  float d0=(bhRadius-d)/bhRadius;
  float dm0 = 1.0-max(d0,0.0);
  float gl = 1.0-clamp(-0.3*d0,0.0,1.0);
  float gla = pow(1.0-min(abs(d0),1.0),14.0);
  float gl8 = pow(gl,12.0);
  float hole = 0.9*pow(dm0,42.0)+0.1*pow(dm0,5.0);
  float bh = (gla+0.8*gl8+0.2*gl8*gl8)*hole;
  df *= 0.9+0.1*sin(8.0*vd.z+d+4.0*t-4.0*df);
  bh *= 1.0+pow(df,4.0)*hole*max(1.0-bh,0.0);
  vec3 col = bh*4.0*mix(NL_BH_COL_LOW,NL_BH_COL_HIGH,min(bh,1.0));
  // hotter accretion ring outside the core
  float ringRadius=0.72*bhRadius;
  float ringWidth=0.30*bhRadius;
  float ring=pow(max(1.0-abs(d-ringRadius)/max(ringWidth,0.001),0.0),3.0);
  col += ring*mix(NL_END_HORIZON_COL,NL_END_ZENITH_COL,0.35)*0.8;
  return vec4(col,hole);
}

vec3 renderEndSky(vec3 horizonCol,vec3 zenithCol,vec3 viewDir,float t) {
  float a = atan2(viewDir.x,viewDir.z);
  float tt = t*0.035;
  float n0 = 0.5+0.5*sin(2.0*a+tt+4.0*viewDir.y);
  float n1 = 0.5+0.5*sin(4.0*a-0.7*tt+5.0*n0);
  float n2 = 0.5+0.5*sin(9.0*a+1.1*tt+7.0*n1+3.0*viewDir.x);
  float neb = n0*n1;
  float band = pow(max(1.0-abs(viewDir.y+0.12),0.0),2.0);
  float horizon = smoothstep(0.9,-0.35,viewDir.y);

  vec3 violet = NL_END_ZENITH_COL;
  vec3 magenta = NL_END_HORIZON_COL;
  vec3 blue = mix(NL_END_ZENITH_COL,NL_END_HORIZON_COL,0.5);
  vec3 sky = mix(zenithCol,horizonCol,horizon);
  sky += neb*band*mix(violet,magenta,n1)*1.8;
  sky += n2*n2*band*blue*0.7;

  // distant star field
  float stars = noise3D(viewDir*180.0+t*0.002);
  stars = smoothstep(0.78,0.96,stars);
  float starVar = 0.5+0.5*sin(31.0*viewDir.x+17.0*viewDir.z);
  sky += stars*(0.7+0.7*starVar)*mix(NL_NIGHT_EDGE_COL,NL_END_HORIZON_COL,n1)*1.6;
  return sky;
}

vec3 nlRenderSky(nl_skycolor skycol,nl_environment env,vec3 viewDir,float t,bool isSkyPlane) {
  vec3 sky;
  viewDir.y = -viewDir.y;
  if (env.end) {
    sky = renderEndSky(skycol.horizon,skycol.zenith,viewDir,t);
    #ifdef NL_BLACKHOLE
      vec4 bh = renderBlackhole(viewDir,t);
      sky *= bh.a;
      sky += bh.rgb;
    #endif
  } else {
    sky = renderOverworldSky(skycol,env,viewDir,isSkyPlane);
    #ifdef NL_UNDERWATER_STREAKS
      if (env.underwater) {
        float a = atan2(viewDir.x,viewDir.z);
        float grad = pow(max(0.5+0.5*viewDir.y,0.0),2.0);
        float spread = 0.5+0.5*sin(3.0*a+0.2*t+2.0*sin(5.0*a-0.4*t));
        spread *= 0.5+0.5*sin(3.0*a-sin(0.5*t));
        spread = mix(spread,1.0,grad);
        spread = spread*spread;
        sky += 2.0*spread*grad*skycol.horizon;
      }
    #endif
  }
  return sky;
}

vec3 nlRenderShootingStar(vec3 viewDir,vec3 FOG_COLOR,float t) {
  float h=t/(NL_SHOOTING_STAR_DELAY+NL_SHOOTING_STAR_PERIOD);
  float h0=floor(h);
  t=(NL_SHOOTING_STAR_DELAY+NL_SHOOTING_STAR_PERIOD)*(h-h0);
  t=min(t/NL_SHOOTING_STAR_PERIOD,1.0);
  float t0=t*t;
  float t1=1.0-t0;
  t1*=t1;t1*=t1;t1*=t1;
  float r=fract(sin(h0)*43758.545313);
  float a=6.2831*r;
  float cosa=cos(a),sina=sin(a);
  vec2 uv=viewDir.xz*(6.0+4.0*r);
  uv=vec2(cosa*uv.x+sina*uv.y,-sina*uv.x+cosa*uv.y);
  uv.x+=t1-t;
  uv.x-=2.0*r+3.5;
  uv.y+=viewDir.y*3.0;
  float g=1.0-min(abs((uv.x-0.95))*20.0,1.0);
  float s=1.0-min(abs(8.0*uv.y),1.0);
  s*=s*s*smoothstep(-1.0+1.96*t1,0.98-t,uv.x);
  s*=s*s*smoothstep(1.0,0.98-t0,uv.x);
  s*=1.0-t1;s*=1.0-t0;
  s*=0.7+16.0*g*g;
  s*=max(1.0-FOG_COLOR.r-FOG_COLOR.g-FOG_COLOR.b,0.0);
  return s*mix(NL_NIGHT_EDGE_COL,NL_NOON_SUNLIGHT_COL,0.35);
}

vec3 nlRenderGalaxy(vec3 vdir,vec3 fogColor,nl_environment env,float t) {
  if (env.underwater) return vec3_splat(0.0);
  t*=NL_GALAXY_SPEED;
  float ca=cos(0.2*t),sa=sin(0.2*t);
  vdir.xy=mul(mat2(ca,sa,-sa,ca),vdir.xy);
  float n0=0.5+0.5*sin(5.0*vdir.x)*sin(5.0*vdir.y-0.5*t)*sin(5.0*vdir.z+0.5*t);
  float n1=noise3D(15.0*vdir+sin(0.85*t+1.3));
  float n2=noise3D(50.0*vdir+1.0*n1+sin(0.7*t+1.0));
  float n3=noise3D(200.0*vdir-10.0*sin(0.4*t+0.5));
  n3=smoothstep(0.04,0.3,n3+0.02*n2);
  float gd=vdir.x+0.1*vdir.y+0.1*sin(10.0*vdir.z+0.2*t);
  float st=n1*n2*n3*n3*(1.0+70.0*gd*gd);
  st=(1.0-st)/(1.0+400.0*st);
  vec3 stars=(0.8+0.2*sin(vec3(8.0,6.0,10.0)*(2.0*n1+0.8*n2)+vec3(0.0,0.4,0.82)))*st*mix(NL_NIGHT_EDGE_COL,NL_NIGHT_HORIZON_COL,0.5);
  float gfmask=abs(vdir.x)-0.15*n1+0.04*n2+0.25*n0;
  float gf=1.0-(vdir.x*vdir.x+0.03*n1+0.2*n0);
  gf*=gf;gf*=gf*gf;
  gf*=1.0-0.3*smoothstep(0.2,0.3,gfmask);
  gf*=1.0-0.2*smoothstep(0.3,0.4,gfmask);
  vec3 gfcol=normalize(vec3(n0,cos(2.0*vdir.y),sin(vdir.x+n0)));
  stars+=(0.4*gf+0.012)*mix(vec3_splat(0.5),gfcol*gfcol,NL_GALAXY_VIBRANCE);
  stars*=mix(1.0,NL_GALAXY_DAY_VISIBILITY,env.dayFactor);
  return stars*(1.0-env.rainFactor);
}

#endif
