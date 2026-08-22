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
  vec3 s = vec3(x-0.5, x, x+0.5);
  s = smoothstep(1.0,0.0,abs(s));
  return s*s;
}

vec3 getUnderwaterCol(vec3 FOG_COLOR) {
  return 2.0*NL_UNDERWATER_TINT*FOG_COLOR*FOG_COLOR;
}

vec3 getEndZenithCol() {
  return NL_END_ZENITH_COL;
}

vec3 getEndHorizonCol() {
  return NL_END_HORIZON_COL;
}

nl_skycolor nlEndSkyColors(nl_environment env) {
  nl_skycolor s;
  s.zenith = getEndZenithCol();
  s.horizon = getEndHorizonCol();
  s.horizonEdge = mix(s.horizon,s.zenith,0.18);
  return s;
}

nl_skycolor nlOverworldSkyColors(nl_environment env) {
  nl_skycolor s;
  float day = clamp(env.dayFactor,-1.0,1.0);
  float night = smoothstep(0.05,-0.35,day);
  float dawn = 1.0-day*day;
  dawn *= dawn;
  dawn *= dawn;
  dawn *= mix(1.0,dawn*dawn,night);
  float nightScale = 1.0+1.65*smoothstep(0.0,-1.0,day);

  s.zenith = mix(NL_DAY_ZENITH_COL,NL_NIGHT_ZENITH_COL*nightScale,night);
  s.horizon = mix(NL_DAY_HORIZON_COL,NL_NIGHT_HORIZON_COL*nightScale,night);
  s.horizonEdge = mix(NL_DAY_EDGE_COL,NL_NIGHT_EDGE_COL*nightScale,night);

  s.zenith = mix(s.zenith,NL_DAWN_ZENITH_COL,dawn);
  s.horizon = mix(s.horizon,NL_DAWN_HORIZON_COL,dawn);
  s.horizonEdge = mix(s.horizonEdge,NL_DAWN_EDGE_COL,dawn);

  float rain = env.rainFactor*NL_SKY_RAIN_MIX_FACTOR;
  float zLum = dot(s.zenith,vec3_splat(0.333333));
  float hLum = dot(s.horizon,vec3_splat(0.333333));
  s.zenith = mix(s.zenith,NL_RAIN_ZENITH_COL*zLum,rain);
  s.horizon = mix(s.horizon,NL_RAIN_HORIZON_COL*hLum,rain);
  s.horizonEdge = mix(s.horizonEdge,mix(NL_RAIN_HORIZON_COL,NL_RAIN_ZENITH_COL,0.18)*hLum,env.rainFactor*0.72);

  if (env.underwater) {
    vec3 underwaterFog = env.fogCol*env.fogCol*NL_UNDERWATER_TINT;
    s.zenith = mix(2.0*underwaterFog,underwaterFog*zLum,0.78);
    s.horizon = mix(2.0*underwaterFog,underwaterFog*hLum,0.78);
    s.horizonEdge = s.horizon;
  }

  return s;
}

nl_skycolor nlSkyColors(nl_environment env) {
  if (env.end) {
    return nlEndSkyColors(env);
  }
  return nlOverworldSkyColors(env);
}

vec3 renderOverworldSky(nl_skycolor skyCol, nl_environment env, vec3 viewDir, bool isSkyPlane) {
  float vy = clamp(viewDir.y,-1.0,1.0);
  float horizon = 1.0-abs(vy);
  float horizonWide = smoothstep(0.0,0.98,horizon);
  float upper = smoothstep(-0.18,0.78,vy);
  float lower = smoothstep(0.25,-0.75,vy);

  vec2 sunMoon = clamp(0.5-0.5*vec2(dot(env.sunDir,viewDir),dot(env.moonDir,viewDir)),0.0,1.0);
  vec2 g = 1.0-mix(sqrt(sunMoon),sunMoon,env.rainFactor);
  vec2 g2 = g*g;
  vec2 g4 = g2*g2;
  vec2 g8 = g4*g4;
  float source = (g8.x+g8.y)*0.5*(1.0-0.92*env.rainFactor);

  float vertical = smoothstep(-0.72,0.82,vy);
  float horizonBlend = pow(horizonWide,1.15);
  float edgeBlend = pow(horizonWide,2.4);
  vec3 sky = mix(skyCol.zenith,skyCol.horizon,vertical);
  sky = mix(sky,skyCol.horizonEdge,horizonBlend*0.68);
  sky = mix(sky,skyCol.horizonEdge,edgeBlend*0.22*(1.0-upper));

  float dawn = 1.0-env.dayFactor*env.dayFactor;
  dawn *= dawn*dawn;
  dawn *= mix(1.0,dawn*dawn,step(env.dayFactor,0.0));
  float sunHorizon = (1.0-smoothstep(0.02,0.78,abs(env.sunDir.y)))*horizonWide;
  float dawnHaze = dawn*sunHorizon*(1.0-0.72*env.rainFactor);
  vec3 dawnCol = mix(NL_DAWN_HORIZON_COL,NL_DAWN_EDGE_COL,smoothstep(0.05,0.95,horizonWide));
  sky += dawnCol*(0.055*dawnHaze+0.035*dawnHaze*dawnHaze);

  float sunAureole = exp(-2.4*sqrt(max(sunMoon.x,0.0)));
  float sunCore = exp(-22.0*sunMoon.x);
  float noonFactor = smoothstep(0.12,0.62,max(env.dayFactor,0.0));
  float warmFactor = dawn*(1.0-noonFactor);
  vec3 sunCol = mix(NL_NOON_SUNLIGHT_COL,NL_DAWN_SUNLIGHT_COL,warmFactor);
  float halo = (0.075*sunAureole+0.62*sunCore)*(1.0-env.rainFactor*0.88);
  halo *= 0.48+0.52*horizonWide;
  sky += sunCol*halo;

  float broadHalo = exp(-0.65*sqrt(max(sunMoon.x,0.0)));
  broadHalo *= 0.045*(1.0-env.rainFactor)*(0.45+0.55*dawn);
  sky += sunCol*broadHalo*horizonWide;

  float moonHalo = exp(-8.0*sunMoon.y)*(1.0-dawn)*(1.0-env.rainFactor);
  sky += NL_NIGHT_MOONLIGHT_COL*(0.045*moonHalo);

  if (!isSkyPlane) {
    float src = smoothstep(0.05,0.88,source);
    src *= src;
    sky *= 1.0+(6.0+9.0*src)*src*(1.0-env.rainFactor*0.9);
    sky += sunCol*(0.035*src+0.12*src*src);
  }

  float voidMask = smoothstep(0.05,-0.82,vy);
  float voidFade = mix(1.0,1.0-0.42*NL_SKY_VOID_DARKNESS,voidMask*NL_SKY_VOID_FACTOR);
  sky *= voidFade;
  sky *= 0.92+0.08*upper+0.035*horizonWide;

  #ifdef NL_RAINBOW
    float rainbowFade = pow(smoothstep(-0.05,0.88,vy),2.0);
    rainbowFade *= mix(NL_RAINBOW_CLEAR,NL_RAINBOW_RAIN,env.rainFactor);
    rainbowFade *= 0.5+0.5*max(env.dayFactor,0.0);
    sky += spectrum(24.2*(0.85-sunMoon.x))*rainbowFade*skyCol.horizon*0.42;
  #endif

  return max(sky,vec3_splat(0.0));
}

vec4 renderBlackhole(vec3 vdir, float t) {
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

  // blackhole distance
  float d = length(vd);

  float nl = sin(15.0*vd.x+t)*sin(15.0*vd.y-t)*sin(15.0*vd.z+t);

  float df = sin(3.0*vd.x-4.0*d+24.0*pow(1.4-d,4.0)+t);

  float d0 = (0.6-d)/0.6;
  float dm0 = 1.0-max(d0,0.0);

  float gl = 1.0-clamp(-0.3*d0,0.0,1.0);

  float gla = pow(1.0-min(abs(d0),1.0),14.0);

  float gl8 = pow(gl,12.0);

  float hole = 0.9*pow(dm0,42.0)+0.1*pow(dm0,5.0);

  float bh = (gla+0.8*gl8+0.2*gl8*gl8)*hole;

  df *= 0.9+0.1*sin(8.0*vd.z+d+4.0*t - 4.0*df);

  bh *= 1.0+pow(df,4.0)*hole*max(1.0-bh,0.0);

  vec3 col = bh*4.0*mix(NL_BH_COL_LOW,NL_BH_COL_HIGH,min(bh,1.0));

  return vec4(col,hole);
}

vec3 renderEndSky(vec3 horizonCol, vec3 zenithCol, vec3 viewDir, float t) {
  float time = t*0.018*NL_BH_SPEED;
  float vy = clamp(viewDir.y,-1.0,1.0);
  float a = atan2(viewDir.x,viewDir.z);
  float horizon = 1.0-abs(vy);
  float horizonFade = smoothstep(-0.75,0.7,vy);
  float bandCenter = 0.12*sin(2.0*a+time*0.35)+0.04*sin(7.0*a-time*0.8);
  float band = exp(-24.0*(vy-bandCenter)*(vy-bandCenter));
  float bandWide = exp(-7.0*(vy-bandCenter)*(vy-bandCenter));

  float n0 = 0.5+0.5*sin(4.0*a+time+2.2*viewDir.x*viewDir.y);
  float n1 = 0.5+0.5*sin(9.0*a-time*0.7+5.0*n0+0.3*sin(32.0*a+time));
  float n2 = noise3D(7.0*viewDir+vec3(time*0.15,-time*0.08,time*0.12));
  float n3 = noise3D(26.0*viewDir+vec3(-time*0.25,time*0.14,time*0.18));
  float cloud = smoothstep(0.24,0.82,n2*n3+0.18*n1);

  vec3 sky = mix(zenithCol,horizonCol,smoothstep(-0.82,0.72,vy));
  sky += horizonCol*(0.10*bandWide+0.16*band*cloud);
  sky += zenithCol*(0.12*n0*n0*(1.0-horizonFade));

  float starNoise = noise3D(150.0*viewDir+vec3(time*0.05,-time*0.03,time*0.07));
  float stars = smoothstep(0.74,0.985,starNoise);
  stars *= 0.25+0.75*smoothstep(-0.15,0.8,abs(vy));
  stars *= 0.32+0.68*(1.0-band*0.65);
  float starTwinkle = 0.72+0.28*sin(18.0*starNoise+time*2.4);
  vec3 starCol = mix(NL_BH_COL_HIGH,NL_END_HORIZON_COL,0.18+0.28*noise3D(8.0*viewDir));
  sky += starCol*stars*starTwinkle*0.55;

  float galactic = exp(-3.2*abs(viewDir.x+0.16*sin(2.0*a+time)));
  galactic *= 0.18+0.82*noise3D(4.0*viewDir+time*0.04);
  sky += mix(zenithCol,horizonCol,0.45)*galactic*0.16;

  #ifdef NL_BLACKHOLE
    vec4 bh = renderBlackhole(viewDir, t);
    sky *= bh.a;
    sky += bh.rgb;
  #endif

  return max(sky,vec3_splat(0.0));
}

vec3 nlRenderSky(nl_skycolor skycol, nl_environment env, vec3 viewDir, float t, bool isSkyPlane) {
  vec3 sky;
  viewDir.y = -viewDir.y;
  if (env.end) {
    sky = renderEndSky(skycol.horizon,skycol.zenith,viewDir,t);
  } else {
    sky = renderOverworldSky(skycol,env,viewDir,isSkyPlane);
    #ifdef NL_UNDERWATER_STREAKS
    #endif
  }
  return sky;
}

vec3 nlRenderShootingStar(vec3 viewDir, vec3 FOG_COLOR, float t) {
  float h = t/(NL_SHOOTING_STAR_DELAY+NL_SHOOTING_STAR_PERIOD);
  float h0 = floor(h);
  t = (NL_SHOOTING_STAR_DELAY+NL_SHOOTING_STAR_PERIOD)*(h-h0);
  t = min(t/NL_SHOOTING_STAR_PERIOD,1.0);
  float t0 = t*t;
  float t1 = 1.0-t0;
  t1 *= t1; t1 *= t1; t1 *= t1;
  float r = fract(sin(h0)*43758.545313);
  float a = 6.2831*r;
  float cosa = cos(a);
  float sina = sin(a);
  vec2 uv = viewDir.xz*(6.0+4.0*r);
  uv = vec2(cosa*uv.x+sina*uv.y,-sina*uv.x+cosa*uv.y);
  uv.x += t1-t;
  uv.x -= 2.0*r+3.5;
  uv.y += viewDir.y*3.0;
  float g = 1.0-min(abs((uv.x-0.95))*20.0,1.0);
  float s = 1.0-min(abs(8.0*uv.y),1.0);
  s *= s*s*smoothstep(-1.0+1.96*t1,0.98-t,uv.x);
  s *= s*s*smoothstep(1.0,0.98-t0,uv.x);
  s *= 1.0-t1;
  s *= 1.0-t0;
  s *= 0.55+18.0*g*g;
  s *= max(1.0-FOG_COLOR.r-FOG_COLOR.g-FOG_COLOR.b,0.0);
  return s*mix(vec3_splat(1.0),FOG_COLOR,0.12);
}

vec3 nlRenderGalaxy(vec3 vdir, vec3 fogColor, nl_environment env, float t) {
  if (env.underwater) return vec3_splat(0.0);
  t *= NL_GALAXY_SPEED;
  float cb = sin(0.2*t);
  float sb = cos(0.2*t);
  vdir.xy = mul(mat2(cb,sb,-sb,cb),vdir.xy);
  float n0 = 0.5+0.5*sin(5.0*vdir.x)*sin(5.0*vdir.y-0.5*t)*sin(5.0*vdir.z+0.5*t);
  float n1 = noise3D(15.0*vdir+sin(0.85*t+1.3));
  float n2 = noise3D(50.0*vdir+1.0*n1+sin(0.7*t+1.0));
  float n3 = noise3D(200.0*vdir-10.0*sin(0.4*t+0.5));
  n3 = smoothstep(0.04,0.3,n3+0.02*n2);
  float gd = vdir.x+0.1*vdir.y+0.1*sin(10.0*vdir.z+0.2*t);
  float st = n1*n2*n3*n3*(1.0+70.0*gd*gd);
  st = (1.0-st)/(1.0+400.0*st);
  vec3 stars = (0.8+0.2*sin(vec3(8.0,6.0,10.0)*(2.0*n1+0.8*n2)+vec3(0.0,0.4,0.82)))*st;
  float gfmask = abs(vdir.x)-0.15*n1+0.04*n2+0.25*n0;
  float gf = 1.0-(vdir.x*vdir.x+0.03*n1+0.2*n0);
  gf *= gf; gf *= gf*gf;
  gf *= 1.0-0.3*smoothstep(0.2,0.3,gfmask);
  gf *= 1.0-0.2*smoothstep(0.3,0.4,gfmask);
  gf *= 1.0-0.1*smoothstep(0.2,0.1,gfmask);
  vec3 gfcol = normalize(vec3(n0,cos(2.0*vdir.y),sin(vdir.x+n0)));
  stars += (0.4*gf+0.012)*mix(vec3_splat(0.5),gfcol*gfcol,NL_GALAXY_VIBRANCE);
  stars *= mix(1.0,NL_GALAXY_DAY_VISIBILITY,env.dayFactor);
  return stars*(1.0-env.rainFactor);
}

#endif
