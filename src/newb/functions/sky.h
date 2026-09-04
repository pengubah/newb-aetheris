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

  float dawnFactor = 1.0-env.dayFactor*env.dayFactor;
  dawnFactor = smoothstep(0.0,1.0,dawnFactor);
  dawnFactor *= dawnFactor;
  dawnFactor *= dawnFactor;
  dawnFactor *= mix(1.0,dawnFactor*dawnFactor,nightFactor);
  float dawnZenith = smoothstep(0.02,0.82,dawnFactor);
  float dawnHorizon = smoothstep(0.0,0.72,dawnFactor);
  float dawnEdge = smoothstep(0.0,0.48,dawnFactor);
  s.zenith = mix(s.zenith,NL_DAWN_ZENITH_COL,dawnZenith);
  s.horizon = mix(s.horizon,NL_DAWN_HORIZON_COL,dawnHorizon);
  s.horizonEdge = mix(s.horizonEdge,NL_DAWN_EDGE_COL,dawnEdge);

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

  float dawnFactor = 1.0-env.dayFactor*env.dayFactor;
  float df = mix(1.0,g2.x,dawnFactor*dawnFactor);
  float dawnBlend = smoothstep(0.0,1.0,dawnFactor);
  float dawnGradient = mix(gradient2,pow(gradient2,0.6),dawnBlend);
  vec3 sky = mix(skyCol.horizon,skyCol.horizonEdge,gradient1*df*df);
  sky = mix(skyCol.zenith,sky,dawnGradient*df);
  float sunDot = max(dot(env.sunDir,viewDir),0.0);
  float sunHorizon = 1.0-smoothstep(0.0,0.42,abs(viewDir.y));
  float sunLow = 1.0-smoothstep(0.0,0.48,abs(env.sunDir.y));
  float dawnAtmosphere = 1.0-env.dayFactor*env.dayFactor;
  dawnAtmosphere = smoothstep(0.0,1.0,dawnAtmosphere);
  dawnAtmosphere *= dawnAtmosphere;
  dawnAtmosphere *= dawnAtmosphere;
  float sunHalo = pow(sunDot,2.0);
  sunHalo *= sunHorizon;
  sunHalo *= sunLow;
  sunHalo *= dawnAtmosphere;
  float sunGlow = pow(sunDot,8.0);
  sunGlow *= sunHorizon;
  sunGlow *= sunLow;
  sunGlow *= dawnAtmosphere;
  float horizonLight = sunHorizon*sunLow*dawnAtmosphere;
  horizonLight *= smoothstep(0.0,0.75,sunDot);
  sky += skyCol.horizonEdge*sunHalo*2.8;
  sky += skyCol.horizon*sunGlow*4.0;
  sky += skyCol.horizonEdge*horizonLight*0.65;

  sky *= 0.5+0.5*gradient2;
  sky *= (1.0 + (2.0*mg8 + 7.0*mg8*mg8)*mask)*mix(1.0, mask, NL_SKY_VOID_DARKNESS);

  if (!isSkyPlane) {
    float source = max(0.0,(mg8-0.22)/0.78);
    source *= source;
    source *= source;
    float dawnSource = source*dawnAtmosphere;
    sky *= 1.0+15.0*source*(1.0-env.rainFactor);
    sky += skyCol.horizonEdge*dawnSource*1.8;
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

vec4 renderBlackhole(vec3 viewdir,float t) {
    t *= NL_BH_SPEED;
    float r = 2.4;
    vec3 vr = viewdir;
    vr.xy = vec2(vr.x * cos(r) - vr.y * sin(r),vr.x * sin(r) + vr.y * cos(r));
    vec3 viewd = vr - vec3(0.0,-1.0,0.0);
    float nl = sin(15.0 * viewd.x + t) * sin(15.0 * viewd.y - t) * sin(15.0 * viewd.z + t);
    float a = atan2(viewd.x,viewd.z);
    float d = NL_BH_DIST * length(viewd + 0.003 * nl);
    float d0 = (0.6 - d) / 0.6;
    float dm0 = 1.0 - max(d0,0.0);
    float gl = 1.0 - clamp(-0.3 * d0,0.0,1.0);
    float gla = pow(1.0 - min(abs(d0),1.0),8.0);
    float gl8 = pow(gl,8.0);
    float hole = 0.9 * pow(dm0,32.0) + 0.1 * pow(dm0,3.0);
    float bh = (gla + 0.8 * gl8 + 0.2 * gl8 * gl8) * hole;
    float spiralPhase = 3.0 * a - 4.0 * d + 24.0 * pow(max(1.4 - d,0.0),4.0) + t;
    float df = sin(spiralPhase);
    df *= 0.9 + 0.1 * sin(8.0 * a + d + 4.0 * t - 4.0 * df);
    float df2 = sin(7.0 * a - 8.0 * d + 13.0 * pow(max(1.25 - d,0.0),3.0) - t * 1.35);
    df += 0.08 * df2;
    bh *= 1.0 + pow(df,4.0) * hole * max(1.0 - bh,0.0);
    float spiralWave = 0.4 + 0.35 * sin(spiralPhase);
    float spiralShape = pow(spiralWave,2.0);
    float innerSpiral = smoothstep(0.18,0.62,d) * spiralShape;
    float innerBreakup = 0.72 + 0.28 * spiralShape;
    float detail = 0.82 + 0.04 * sin(12.0 * a - 15.0 * d + t * 1.5);
    bh *= innerBreakup * detail;
    float edge = pow(max(1.0 - smoothstep(0.34,0.62,d),0.0),3.0);
    float halo = pow(max(1.0 - smoothstep(0.45,1.15,d),0.0),4.0);
    float spiralLight = 0.72 + 0.28 * spiralShape;
    float light = bh * 4.0;
    light += edge * bh * 0.20;
    light += innerSpiral * bh * 0.16;
    light += halo * bh * 0.10;
    light *= spiralLight;
    float colorMix = clamp(bh * (0.72 + 0.28 * spiralShape),0.0,1.0);
    vec3 col = light * mix(NL_BH_COL_LOW,NL_BH_COL_HIGH,colorMix);
    return vec4(col,hole);
}

vec3 renderEndSky(vec3 horizonCol,vec3 zenithCol,vec3 viewDir,float t) {
    float skyTime = t*1.5;
    float a = atan2(viewDir.x,viewDir.z);
    vec3 dir = normalize(viewDir);
    float grad = 0.5+0.5*dir.y;
    float horizon = 1.0-smoothstep(-0.25,0.8,dir.y);
    vec3 sky = mix(zenithCol,horizonCol,pow(1.0-grad,1.35));

    vec3 p = dir*2.65;
    p += vec3(skyTime*0.045,skyTime*0.0,-skyTime*0.0);

    float warpA = noise3(p*0.58+vec3(5.2,13.1,8.7));
    float warpB = noise3(p*0.58+vec3(17.4,4.6,11.8));

    vec3 warped = p;
    warped += vec3(warpA-0.5,warpB-0.5,warpA+warpB-1.0)*0.72;

    float cloudLarge = fbm3(warped*0.72);
    float cloudMedium = fbm3(warped*1.45+vec3(12.7,6.3,19.4));
    float cloudDetail = noise3(warped*2.75+vec3(4.8,17.2,9.6));

    float cloudShape = cloudLarge*0.62+cloudMedium*0.27+cloudDetail*0.11;
    float cloudMass = smoothstep(0.31,0.56,cloudShape);
    float cloudCore = smoothstep(0.53,0.72,cloudShape);

    float filament = noise3(warped*2.15+vec3(21.3,7.1,14.8));
    float gas = cloudMass*(0.78+0.22*filament);

    vec3 deepPurple = vec3(0.025,0.004,0.065);
    vec3 purpleGas = vec3(0.105,0.008,0.19);
    vec3 warmPurple = vec3(0.19,0.013,0.26);

    vec3 nebulaColor = mix(deepPurple,purpleGas,smoothstep(0.38,0.58,cloudShape));
    nebulaColor = mix(nebulaColor,warmPurple,cloudCore);

    float nebulaStrength = gas*1.3;

    sky += nebulaColor*nebulaStrength;
    sky += vec3(0.055,0.003,0.095)*cloudCore*0.48;

    vec3 starCell = floor(dir*185.0);
    vec3 rnd = hash33(starCell);

    float starChance = step(0.980,rnd.x);
    float starDist = length(fract(dir*185.0)-0.5);
    float starRadius = mix(0.105,0.18,rnd.y);
    float starPoint = 1.0-smoothstep(starRadius,starRadius*1.45,starDist);

    vec3 starColor = vec3(1.0,0.93,0.82);

    if (rnd.x > 0.978) {
        starColor = vec3(0.55,0.72,1.0);
    }
    if (rnd.x > 0.988) {
        starColor = vec3(0.72,0.55,1.0);
    }
    if (rnd.x > 0.995) {
        starColor = vec3(0.55,0.9,1.0);
    }
    float twinkle = 1.0;
    if (rnd.y > 0.92) {
        twinkle = 0.9+0.1*sin(skyTime*1.5+rnd.z*6.28318);
    }
    float star = starPoint*starChance*twinkle;
    star *= 1.0-cloudCore*0.18;
    sky += starColor*star*1.35;
    
    #ifdef NL_BLACKHOLE
      vec4 bh = renderBlackhole(viewDir,t);
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

vec3 nlRenderOverworldStars(vec3 viewDir, nl_environment env) {
    vec3 dir = normalize(viewDir);
    vec3 cell = floor(dir * 260.0);
    vec3 rnd = hash33(cell);
    vec3 local = fract(dir * 260.0);
    vec3 starPos = 0.15 + rnd * 0.70;
    float dist = length(local - starPos);
    float starChance = step(0.25, rnd.x);
    float starPoint = 1.0 - smoothstep(0.040, 0.040 * 1.45, dist);

    float colorId = floor(rnd.z * 6.0);
    vec3 starColor = vec3(1.0, 1.0, 1.0);

    if (colorId < 1.0) {
        starColor = vec3(1.0, 0.0, 1.0);
    } else if (colorId < 2.0) {
        starColor = vec3(0.0, 1.0, 0.0);
    } else if (colorId < 3.0) {
        starColor = vec3(0.0, 1.0, 1.0);
    } else if (colorId < 4.0) {
        starColor = vec3(1.0, 1.0, 0.0);
    } else if (colorId < 5.0) {
        starColor = vec3(1.0, 1.0, 1.0);
    }

    float brightness = mix(0.72, 1.20, rnd.y);
    float star = starPoint * starChance * brightness * 2.0;
    star *= mix(1.0, 0.0, env.dayFactor);
    star *= 1.0 - env.rainFactor;
    
    return starColor * star * NL_OVERWORLD_STARS;
}

#endif
