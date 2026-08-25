#ifndef SKY_H
#define SKY_H

#include "detection.h"
#include "noise.h"

struct nl_skycolor {
  vec3 zenith;
  vec3 horizon;
  vec3 horizonEdge;
};

// rainbow spectrum
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

vec4 renderBlackhole(vec3 viewdir, float t) {
  t *= NL_BH_SPEED;

  float r = 2.4;
  vec3 vr = viewdir;

  float cr = cos(r);
  float sr = sin(r);

  vec2 rot;
  rot.x = cr*vr.x-sr*vr.y;
  rot.y = sr*vr.x+cr*vr.y;
  vr.xy = rot;

  vec3 viewd = vr-vec3(0.0,-1.0,0.0);

  float d = length(viewd);

  float nl = sin(15.0*viewd.x+t)*sin(15.0*viewd.y-t)*sin(15.0*viewd.z+t);

  float df = sin(3.0*viewd.x-4.0*d+24.0*pow(1.4-d,4.0)+t);

  float spiral = sin(18.0*atan2(viewd.z,viewd.x)-9.0*d+3.0*t);

  df *= 0.92+0.08*sin(8.0*viewd.z+d+4.0*t-4.0*df);

  df += spiral*0.035;

  float d0 = (0.6-d)/0.6;

  float dm0 = 1.0-max(d0,0.0);

  float gl = 1.0-clamp(-0.3*d0,0.0,1.0);

  float gla = pow(1.0-min(abs(d0),1.0),20.0);

  float gl8 = pow(gl,16.0);

  float hole = 0.96*pow(dm0,58.0)+0.04*pow(dm0,14.0);

  float bh = (gla+0.78*gl8+0.16*gl8*gl8)*hole;

  float spiralLight = 0.88+0.12*pow(max(spiral,0.0),2.0);

  bh *= spiralLight;

  bh *= 1.0+pow(df,4.0)*hole*max(1.0-bh,0.0);

  vec3 col = bh*4.0*mix(NL_BH_COL_LOW,NL_BH_COL_HIGH,min(bh,1.0));

  return vec4(col,hole);
}

nl_skycolor nlEndSkyColors(nl_environment env) {
  nl_skycolor s;
  s.zenith = getEndZenithCol();
  s.horizon = getEndHorizonCol();
  s.horizonEdge = s.horizon;
  return s;
}

nl_skycolor nlOverworldSkyColors(nl_environment env) {
  nl_skycolor s;
  float f = 1.0 + 2.0*(1.0-max(-env.dayFactor, 0.0));
  float nightFactor = step(env.dayFactor, 0.0);
  s.zenith = mix(NL_DAY_ZENITH_COL, NL_NIGHT_ZENITH_COL*f, nightFactor);
  s.horizon = mix(NL_DAY_HORIZON_COL, NL_NIGHT_HORIZON_COL*f, nightFactor);
  s.horizonEdge = mix(NL_DAY_EDGE_COL, NL_NIGHT_EDGE_COL*f, nightFactor);

  // Dawn: keep the configured NL_DAWN_* colors, but spread the transition
  // wider so the sky becomes a soft warm gradient instead of a hard orange band.
  float dawnFactor = 1.0-env.dayFactor*env.dayFactor;
  dawnFactor = smoothstep(0.0,1.0,dawnFactor);
  dawnFactor *= dawnFactor;
  dawnFactor *= mix(1.0,dawnFactor*dawnFactor,nightFactor);

  vec3 dawnZenith = NL_DAWN_ZENITH_COL;
  vec3 dawnHorizon = NL_DAWN_HORIZON_COL;
  vec3 dawnEdge = NL_DAWN_EDGE_COL;

  s.zenith = mix(s.zenith,dawnZenith,dawnFactor*0.28);
  s.horizon = mix(s.horizon,dawnHorizon,dawnFactor*0.95);
  s.horizonEdge = mix(s.horizonEdge,dawnEdge,dawnFactor*1.08);

  float zh = dot(s.zenith, vec3_splat(0.33));
  float hh = dot(s.horizon, vec3_splat(0.33));
  float rainMix = env.rainFactor*NL_SKY_RAIN_MIX_FACTOR;
  s.zenith = mix(s.zenith, NL_RAIN_ZENITH_COL*zh, rainMix);
  s.horizon = mix(s.horizon, NL_RAIN_HORIZON_COL*hh, rainMix);
  s.horizonEdge = mix(s.horizonEdge, s.horizon, env.rainFactor);

  if (env.underwater) {
    vec3 underwaterFog = env.fogCol*env.fogCol*NL_UNDERWATER_TINT;
    s.zenith = mix(2.0*underwaterFog, underwaterFog*zh, 0.8);
    s.horizon = mix(2.0*underwaterFog, underwaterFog*hh, 0.8);
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
  float avy = abs(viewDir.y);
  float mask = 0.5 + (0.5*viewDir.y/(0.4 + avy));

  vec2 g = clamp(0.5 - 0.5*vec2(dot(env.sunDir, viewDir), dot(env.moonDir, viewDir)), 0.0, 1.0);
  vec2 g1 = 1.0-mix(sqrt(g), g, env.rainFactor);
  vec2 g2 = g1*g1;
  vec2 g4 = g2*g2;
  vec2 g8 = g4*g4;
  float mg8 = (g8.x+g8.y)*mask*(1.0-0.9*env.rainFactor);

  float vh = 1.0 - viewDir.y*viewDir.y;
  float vh2 = vh*vh;
  vh2 = mix(vh2, mix(1.0, vh2*vh2, NL_SKY_VOID_FACTOR), step(viewDir.y, 0.0));
  vh2 = mix(vh2, 1.0, mg8);
  float vh4 = vh2*vh2;

  float gradient1 = vh4*vh4;
  float gradient2 = 0.8*gradient1 + 0.2*vh2;
  gradient1 *= gradient1;
  gradient1 = mix(gradient1*gradient1, 1.0, mg8);
  gradient2 = mix(gradient2, 1.0, mg8);

  // Keep Dawn illumination concentrated around the sunrise direction while
  // allowing the warm configured sky colors to cover a broad part of the sky.
  float dawnFactor = 1.0-env.dayFactor*env.dayFactor;
  dawnFactor = smoothstep(0.0,1.0,dawnFactor);
  dawnFactor *= dawnFactor;

  float horizonMask = 1.0-abs(viewDir.y);
  horizonMask = smoothstep(0.08,0.62,horizonMask);
  horizonMask *= horizonMask;
  
  float dawnMask = dawnFactor*horizonMask;
  float dawnDirection = pow(g2.x,1.35);
  float df = mix(1.0,dawnDirection,dawnMask);
  vec3 sky = mix(skyCol.horizon, skyCol.horizonEdge, gradient1*df*df);
  sky = mix(skyCol.zenith, sky, gradient2*df);

  sky *= 0.5+0.5*gradient2;
  sky *= (1.0 + (2.0*mg8 + 7.0*mg8*mg8)*mask)*mix(1.0, mask, NL_SKY_VOID_DARKNESS);

  if (!isSkyPlane) {
    float source = max(0.0, (mg8-0.22)/0.78);
    source *= source;
    source *= source;
    sky *= 1.0 + 17.0*source*(1.0-env.rainFactor);
  }

  #ifdef NL_RAINBOW
    if (!env.underwater) {
      float rainbowFade = 0.5 + 0.5*viewDir.y;
      rainbowFade *= rainbowFade;
      rainbowFade *= mix(NL_RAINBOW_CLEAR, NL_RAINBOW_RAIN, env.rainFactor);
      rainbowFade *= 0.5+0.5*env.dayFactor;
      sky += spectrum(24.2*(0.85-g.x))*rainbowFade*skyCol.horizon;
    }
  #endif

  return sky;
}

vec3 renderEndSky(vec3 horizonCol, vec3 zenithCol, vec3 viewDir, float t) {
  float time = t * 0.12;
  float y = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
  float horizon = 1.0 - abs(viewDir.y);
  horizon = clamp(horizon, 0.0, 1.0);

  float upper = smoothstep(0.0, 1.0, y);
  upper *= upper;

  vec3 sky = mix(horizonCol, zenithCol, upper);

  float a = atan2(viewDir.x, viewDir.z);

  float wave1 = sin(a * 3.0 + time * 2.2 + viewDir.y * 5.0);
  float wave2 = sin(a * 6.0 - time * 1.5 + wave1 * 2.0 + viewDir.y * 9.0);
  float wave3 = sin(a * 12.0 + time * 0.9 + viewDir.y * 15.0);

  float cosmic = wave1 * 0.45 + wave2 * 0.35 + wave3 * 0.20;
  cosmic = cosmic * 0.5 + 0.5;

  vec3 flowPos1 = vec3(viewDir.x * 2.8 + time * 0.55, viewDir.y * 3.2 + sin(time * 0.8) * 0.18, viewDir.z * 2.8);
  vec3 flowPos2 = vec3(viewDir.x * 7.0 - time * 0.35, viewDir.y * 6.0 - time * 0.12, viewDir.z * 7.0);
  vec3 flowPos3 = vec3(viewDir.x * 18.0 + time * 0.22, viewDir.y * 14.0, viewDir.z * 18.0 - time * 0.28);

  float n1 = noise3D(flowPos1);
  float n2 = noise3D(flowPos2);
  float n3 = noise3D(flowPos3);

  float nebula = n1 * 0.50 + n2 * 0.32 + n3 * 0.18;
  nebula = smoothstep(0.28, 0.68, nebula);
  nebula *= 0.25 + 0.75 * horizon;

  float bands = sin(a * 5.0 + viewDir.y * 13.0 + sin(time * 1.2) * 3.0 - time * 2.5);
  bands = bands * 0.5 + 0.5;
  bands = smoothstep(0.55, 0.90, bands);
  bands *= horizon * nebula;

  vec3 cyanNebula = vec3(0.005, 0.25, 0.32);
  cyanNebula *= nebula * 2.0;

  float violetPattern = sin(a * 4.0 - viewDir.y * 8.0 + time * 1.7);
  violetPattern = violetPattern * 0.5 + 0.5;
  violetPattern *= nebula;
  violetPattern = smoothstep(0.25, 0.85, violetPattern);

  vec3 violetNebula = vec3(0.16, 0.015, 0.30);
  violetNebula *= violetPattern * 1.4;

  float center = 1.0 - abs(viewDir.y);
  center = smoothstep(0.05, 0.90, center);
  center *= 0.35 + 0.65 * nebula;

  float pulse = 0.5 + 0.5 * sin(time * 3.5);
  center *= 0.55 + 0.45 * pulse;

  vec3 centerGlow = vec3(0.015, 0.32, 0.38);
  centerGlow *= center * 0.9;

  float filament = abs(sin(a * 18.0 + viewDir.y * 25.0 + n1 * 7.0 - time * 3.0));
  filament = 1.0 - filament;
  filament = smoothstep(0.75, 0.98, filament);
  filament *= nebula * horizon;

  vec3 filamentColor = vec3(0.02, 0.38, 0.42);
  filamentColor *= filament * 0.9;

  sky += cyanNebula;
  sky += violetNebula;
  sky += centerGlow;
  sky += filamentColor;

  sky += vec3(0.01, 0.16, 0.20) * bands * 0.85;

  // End stars: sharper, brighter and more visible point stars.
  float lon = atan2(viewDir.x, viewDir.z) / (2.0*PI) + 0.5;
  float lat = asin(clamp(viewDir.y, -1.0, 1.0)) / PI + 0.5;
  vec2 starGrid = vec2(220.0, 110.0);
  vec2 starPos = vec2(lon, lat) * starGrid;
  vec2 starCell = floor(starPos);
  vec2 starLocal = fract(starPos);

  float starSeed = rand(starCell + vec2(17.13, 41.71));
  float starJitterX = rand(starCell + vec2(83.21, 11.37));
  float starJitterY = rand(starCell + vec2(29.47, 67.83));
  vec2 starCenter = 0.12 + 0.76 * vec2(starJitterX, starJitterY);

  float starDistance = length((starLocal-starCenter)*vec2(1.0,1.12));
  float starRadius = mix(0.010,0.026,rand(starCell+vec2(53.91,7.24)));
  float starPoint = 1.0-smoothstep(starRadius*0.18,starRadius,starDistance);
  starPoint *= step(0.945,starSeed);

  float brightSeed = rand(starCell+vec2(101.7,37.4));
  float brightRadius = mix(0.020,0.040,rand(starCell+vec2(5.6,93.2)));
  float brightPoint = 1.0-smoothstep(brightRadius*0.12,brightRadius,starDistance);
  brightPoint *= step(0.982,brightSeed);

  float superSeed = rand(starCell+vec2(137.4,61.8));
  float superRadius = mix(0.030,0.055,rand(starCell+vec2(72.1,19.6)));
  float superPoint = 1.0-smoothstep(superRadius*0.08,superRadius,starDistance);
  superPoint *= step(0.997,superSeed);

  float starTwinkle = 0.90+0.10*sin(time*1.8+starSeed*6.2831);
  float starVisibility = smoothstep(-0.15,0.45,abs(viewDir.y));
  starVisibility *= 0.82+0.18*(1.0-horizon);

  vec3 starColor = mix(vec3(0.72,0.86,1.0),vec3(1.0,0.90,0.72),brightSeed);
  starColor = mix(starColor,vec3(0.82,0.94,1.0),starSeed*0.35);

  float normalStars = starPoint*1.15;
  float brightStars = brightPoint*2.2;
  float superStars = superPoint*3.8;

  sky += starColor*(normalStars+brightStars+superStars)*starTwinkle*starVisibility;

  float starGlow = brightPoint*brightPoint*0.65+superPoint*superPoint*1.4;
  sky += starColor*starGlow*starVisibility;

  float voidMask = smoothstep(0.12, 0.90, abs(viewDir.y));

  sky *= 0.68 + 0.32 * voidMask;
  sky = max(sky, vec3(0.0, 0.0, 0.0));
  sky *= 1.25;
  
  #ifdef NL_BLACKHOLE
    vec4 bh = renderBlackhole(viewDir, t);
    sky *= bh.a;
    sky += bh.rgb;
  #endif

  return sky;
}

vec3 nlRenderSky(nl_skycolor skycol, nl_environment env, vec3 viewDir, float t, bool isSkyPlane) {
  vec3 sky;
  viewDir.y = -viewDir.y;

  if (env.end) {
    sky = renderEndSky(skycol.horizon, skycol.zenith, viewDir, t);
  } else {
    sky = renderOverworldSky(skycol, env, viewDir, isSkyPlane);
    #ifdef NL_UNDERWATER_STREAKS
      // if (env.underwater) {
      //   float a = atan2(viewDir.x, viewDir.z);
      //   float grad = 0.5 + 0.5*viewDir.y;
      //   grad *= grad;
      //   float spread = (0.5 + 0.5*sin(3.0*a + 0.2*t + 2.0*sin(5.0*a - 0.4*t)));
      //   spread *= (0.5 + 0.5*sin(3.0*a - sin(0.5*t)))*grad;
      //   spread += (1.0-spread)*grad;
      //   float streaks = spread*spread;
      //   streaks *= streaks;
      //   streaks = (spread + 3.0*grad*grad + 4.0*streaks*streaks);
      //   sky += 2.0*streaks*skycol.horizon;
      // }
    #endif
  }

  return sky;
}

// shooting star
vec3 nlRenderShootingStar(vec3 viewDir, vec3 FOG_COLOR, float t) {
  // transition vars
  float h = t/(NL_SHOOTING_STAR_DELAY+NL_SHOOTING_STAR_PERIOD);
  float h0 = floor(h);
  t = (NL_SHOOTING_STAR_DELAY+NL_SHOOTING_STAR_PERIOD)*(h-h0);
  t = min(t/NL_SHOOTING_STAR_PERIOD,1.0);
  float t0 = t*t;
  float t1 = 1.0-t0;
  t1 *= t1;
  t1 *= t1;
  t1 *= t1;

  // random seed
  float r = fract(sin(h0*127.31)*43758.545313);
  float r2 = fract(sin(h0*91.73+17.4)*24634.6345);
  float r3 = fract(sin(h0*53.17+8.2)*17431.7271);

  // random rotation
  float a = 6.2831853*r;
  float cosa = cos(a);
  float sina = sin(a);

  // shooting star coordinates
  vec2 uv = viewDir.xz*(5.5+4.5*r);
  uv = vec2(cosa*uv.x+sina*uv.y,-sina*uv.x+cosa*uv.y);
  uv.y += viewDir.y*(2.6+1.4*r2);
  uv.x += t1-t;
  uv.x -= 2.2*r+3.6;

  // slight trajectory curvature
  uv.y += 0.045*sin(uv.x*3.0+t*5.0+r*6.2831);

  // meteor head position
  float headDist = length(vec2(uv.x-0.92,uv.y*1.8));
  float head = 1.0-smoothstep(0.0,0.075+0.035*r2,headDist);
  head *= head;

  // intense hot core
  float coreDist = length(vec2(uv.x-0.94,uv.y*2.8));
  float core = 1.0-smoothstep(0.0,0.028+0.012*r3,coreDist);
  core *= core;

  // sharp main trail
  float trailWidth = 0.018+0.014*r2;
  float trail = 1.0-smoothstep(trailWidth,trailWidth*0.15,abs(uv.y));
  float trailFade = smoothstep(-0.95+1.15*t1,0.82-t,uv.x);
  trail *= trailFade;

  // soft outer trail
  float softTrail = 1.0-smoothstep(0.12+0.04*r2,0.005,abs(uv.y));
  softTrail *= smoothstep(-1.15+1.25*t1,0.65-t,uv.x);
  softTrail *= 0.45;

  // long fading tail
  float longTrail = 1.0-smoothstep(0.28,0.015,abs(uv.y));
  longTrail *= smoothstep(-2.8+2.0*t1,0.45-t,uv.x);
  longTrail *= 0.16;

  // hot tail concentration
  float tailHeat = 1.0-smoothstep(0.0,1.0,abs(uv.x-0.15));
  tailHeat *= trail;

  // atmospheric glow around meteor
  float haloDist = length(vec2(uv.x-0.88,uv.y*0.75));
  float halo = 1.0-smoothstep(0.02,0.42+0.12*r2,haloDist);
  halo *= 0.22;

  // natural breakup along the tail
  float breakup = 0.5+0.5*sin(uv.x*38.0+t*9.0+r*20.0);
  breakup = smoothstep(0.35,0.9,breakup);
  breakup *= trail*0.16;

  // fade animation
  float appear = smoothstep(0.0,0.10,t);
  float disappear = 1.0-smoothstep(0.78,1.0,t);
  float life = appear*disappear;

  // stronger brightness near the head
  float brightness = trail*1.35+softTrail*0.65+longTrail*0.35;
  brightness += head*3.8;
  brightness += core*5.5;
  brightness += halo;
  brightness += breakup;

  brightness *= life;
  brightness *= NL_SHOOTING_STAR;

  // fade during daylight
  float nightMask = max(1.0-FOG_COLOR.r-FOG_COLOR.g-FOG_COLOR.b,0.0);
  nightMask = smoothstep(0.02,0.32,nightMask);
  brightness *= nightMask;

  // subtle cinematic color variation
  vec3 tailColor = mix(vec3(0.62,0.78,1.0),vec3(0.82,0.90,1.0),0.65+0.35*r2);
  vec3 headColor = mix(vec3(0.82,0.92,1.0),vec3(1.0,0.94,0.78),0.25+0.25*r3);

  vec3 col = tailColor*brightness;
  col += headColor*head*2.8*life*nightMask;
  col += vec3(1.0,0.98,0.88)*core*4.0*life*nightMask;

  return col;
}

// Realistic Milky Way Galaxy
vec3 nlRenderGalaxy(vec3 viewdir, vec3 fogColor, nl_environment env, float t) {
  if (env.underwater) {
    return vec3_splat(0.0);
  }

  t *= NL_GALAXY_SPEED;

  float rot = 0.015*sin(t*0.35);
  float cr = cos(rot);
  float sr = sin(rot);
  viewdir.xz = vec2(cr*viewdir.x-sr*viewdir.z,sr*viewdir.x+cr*viewdir.z);

  float band = exp(-pow(abs(viewdir.y+0.08*sin(viewdir.x*2.7))*5.2,2.0));
  float bandWide = exp(-pow(abs(viewdir.y+0.05*sin(viewdir.z*3.0))*2.6,2.0));
  float galacticMask = band*(0.72+0.28*bandWide);

  float n1 = noise3D(viewdir*5.0+vec3(t*0.08,0.0,t*0.04));
  float n2 = noise3D(viewdir*13.0+vec3(-t*0.04,t*0.025,0.0));
  float n3 = noise3D(viewdir*32.0+vec3(t*0.02,-t*0.03,t*0.015));

  float cloud = n1*0.55+n2*0.30+n3*0.15;
  cloud = smoothstep(0.22,0.78,cloud);

  float dust = noise3D(viewdir*18.0+vec3(2.7,-1.4,4.2));
  dust *= noise3D(viewdir*42.0+vec3(-3.2,5.1,1.7));
  dust = smoothstep(0.30,0.72,dust);

  float dustLane = galacticMask*(0.35+0.65*dust);
  float milky = galacticMask*(0.22+0.78*cloud);
  milky *= 1.0-0.72*dustLane;

  float coreAxis = 1.0-abs(viewdir.x*0.78+viewdir.z*0.22);
  float core = pow(max(coreAxis,0.0),7.0);
  core *= galacticMask;
  core *= 0.45+0.55*cloud;

  vec3 coolDust = vec3(0.015,0.035,0.075);
  vec3 warmDust = vec3(0.16,0.075,0.035);
  vec3 milkyColor = mix(coolDust,warmDust,core);

  vec3 galaxy = milkyColor*milky*1.65;
  galaxy += vec3(0.32,0.20,0.12)*core*0.65;
  galaxy *= 0.75+0.25*NL_GALAXY_VIBRANCE;

  float starNoise = noise3D(viewdir*115.0+vec3(t*0.03,-t*0.02,t*0.025));
  float starsSmall = smoothstep(0.72,0.96,starNoise);
  starsSmall *= 0.35+0.65*galacticMask;

  float starNoise2 = noise3D(viewdir*245.0+vec3(7.3,2.1,-4.7));
  float starsFine = smoothstep(0.82,0.985,starNoise2);
  starsFine *= 0.30+0.70*galacticMask;

  vec3 starCool = vec3(0.68,0.82,1.0);
  vec3 starWarm = vec3(1.0,0.76,0.48);
  float starMix = noise3D(viewdir*31.0);
  vec3 starCol = mix(starCool,starWarm,starMix*0.45);

  galaxy += starCol*starsSmall*NL_GALAXY_STARS*0.95;
  galaxy += starCol*starsFine*NL_GALAXY_STARS*0.55;

  float brightNoise = noise3D(viewdir*360.0+vec3(4.2,-8.1,3.4));
  float brightStars = smoothstep(0.965,0.998,brightNoise);
  brightStars *= 0.45+0.55*galacticMask;

  float starGlow = brightStars*brightStars;
  galaxy += vec3(0.82,0.90,1.0)*brightStars*NL_GALAXY_STARS*2.2;
  galaxy += vec3(0.30,0.42,0.65)*starGlow*NL_GALAXY_STARS*1.5;

  float coreGlow = exp(-pow(abs(viewdir.y)*7.0,2.0));
  coreGlow *= pow(max(1.0-abs(viewdir.x),0.0),5.0);
  galaxy += vec3(0.20,0.12,0.075)*coreGlow*0.45;

  galaxy *= mix(1.0,NL_GALAXY_DAY_VISIBILITY,env.dayFactor);
  galaxy *= 1.0-env.rainFactor;

  float fogFade = 1.0-smoothstep(0.35,0.95,fogColor.r+fogColor.g+fogColor.b);
  galaxy *= 0.55+0.45*fogFade;

  return galaxy;
}


#endif
