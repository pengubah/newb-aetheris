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

// Dawn/dusk strength. 0.0 = full night/day, 1.0 = sun sitting on the horizon.
// The mask is deliberately symmetric so sunrise and sunset use the same palette.
float nlDawnFactor(float dayFactor) {
  float d = abs(dayFactor);
  float f = 1.0 - smoothstep(0.0, 0.58, d);
  f = f*f*(3.0 - 2.0*f);
  return f;
}

// Keep the dawn/sunset horizon intact, but reduce how far its warm influence
// carries into the deeper night. At the horizon (dayFactor ~= 0) this is 1.0,
// while deep night keeps only a subtle residual atmospheric influence.
float nlNightDawnAttenuation(float dayFactor) {
  float nightDepth = smoothstep(0.0, 0.58, max(-dayFactor, 0.0));
  return mix(1.0, 0.28, nightDepth);
}

vec4 renderBlackhole(vec3 viewdir, float t) {
    t *= NL_BH_SPEED;

    float size = max(NL_BLACKHOLE_SIZE,0.01);
    vec2 p = viewdir.xz;
    float vertical = viewdir.y;
    float dist = length(p)/size;
    float ang = atan2(p.y,p.x);
    vec3 col = vec3(0.0, 0.0, 0.0);

    float blackCore = smoothstep(0.160,0.132,dist);

    float disk = exp(-pow((dist-0.190)/0.017,2.0));
    float innerDisk = exp(-pow((dist-0.166)/0.012,2.0));
    float outerDisk = exp(-pow((dist-0.210)/0.020,2.0));

    float spiral1 = sin(ang*7.0-dist*105.0+t*1.45);
    float spiral2 = sin(ang*13.0-dist*145.0-t*0.90+spiral1*1.2);
    float spiral3 = sin(ang*21.0-dist*190.0+t*0.55);

    float spiral = spiral1*0.50+spiral2*0.32+spiral3*0.18;
    spiral = spiral*0.5+0.5;
    spiral = smoothstep(0.42,0.78,spiral);

    vec2 noisePos = vec2(cos(ang),sin(ang))*9.0;
    noisePos += vec2(dist*45.0-t*0.35,dist*30.0+t*0.20);

    float turbulence = noise3D(vec3(noisePos,dist));

    spiral *= 0.72+0.38*turbulence;

    float diskLight = disk*(0.38+0.82*spiral);
    diskLight += innerDisk*(0.35+0.65*spiral);
    diskLight += outerDisk*spiral*0.42;

    float purpleGlow = exp(-pow((dist-0.203)/0.036,2.0));
    purpleGlow *= 0.35+0.65*spiral;

    float whiteGlow = exp(-pow((dist-0.166)/0.017,2.0));
    whiteGlow *= 0.55+0.45*spiral;

    vec3 deepPurple = vec3(0.12,0.005,0.30);
    vec3 royalPurple = vec3(0.38,0.025,0.78);
    vec3 violet = vec3(0.68,0.12,1.00);
    vec3 whiteViolet = vec3(0.90,0.72,1.00);
    vec3 white = vec3(1.0,0.97,1.0);

    col += deepPurple*purpleGlow*1.10;
    col += royalPurple*diskLight*1.15;
    col += violet*diskLight*0.70;
    col += whiteViolet*whiteGlow*1.20;
    col += white*innerDisk*0.80;

    float filament = smoothstep(0.62,0.90,spiral);
    filament *= disk;

    col += vec3(0.78,0.22,1.0)*filament*0.60;

    float aura = exp(-pow((dist-0.160)/0.026,2.0));

    col += vec3(0.42,0.06,0.82)*aura*0.38;

    col = mix(col,vec3(0.0, 0.0, 0.0),blackCore);
    col *= 0.88;
    col = col/(1.0+col*0.38);

    float visibility = smoothstep(0.02,0.20,abs(vertical));

    col *= visibility;

    return vec4(col,visibility);
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
  float f = 0.68 + 0.30*(1.0-max(-env.dayFactor, 0.0));
  float nightFactor = nlNightFactor(env.dayFactor);
  s.zenith = mix(NL_DAY_ZENITH_COL, NL_NIGHT_ZENITH_COL*f, nightFactor);
  s.horizon = mix(NL_DAY_HORIZON_COL, NL_NIGHT_HORIZON_COL*f, nightFactor);
  s.horizonEdge = mix(NL_DAY_EDGE_COL, NL_NIGHT_EDGE_COL*f, nightFactor);

  // Dawn/dusk palette. Keep the colors entirely controlled by NL_CONFIG_H;
  // only the transition shape is changed here. This gives a broad purple ->
  // pink/orange -> gold gradient like a real low-sun atmosphere.
  float dawnFactor = nlDawnFactor(env.dayFactor);
  float dawnSoft = dawnFactor*dawnFactor*(3.0-2.0*dawnFactor);
  dawnSoft *= nlNightDawnAttenuation(env.dayFactor);

  s.zenith = mix(s.zenith, NL_DAWN_ZENITH_COL, dawnSoft*0.92);
  s.horizon = mix(s.horizon, NL_DAWN_HORIZON_COL, dawnSoft);
  s.horizonEdge = mix(s.horizonEdge, NL_DAWN_EDGE_COL, dawnSoft);

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

  // Dawn sky shaping. Keep the configured dawn palette intact, but reduce
  // the normal daytime angular gradient while the sun is near the horizon.
  // This prevents a flat blue/orange split and creates the soft layered look
  // seen during golden hour.
  float dawnFactor = nlDawnFactor(env.dayFactor);
  float dawnShape = dawnFactor*dawnFactor*(3.0-2.0*dawnFactor);
  dawnShape *= nlNightDawnAttenuation(env.dayFactor);
  float df = mix(1.0, clamp(g2.x, 0.0, 1.0), dawnShape*0.72);

  vec3 sky = mix(skyCol.horizon, skyCol.horizonEdge, gradient1*df*df);
  sky = mix(skyCol.zenith, sky, gradient2*df);

  // Broad atmospheric glow around the low sun. The glow is intentionally
  // derived from the configured dawn colors instead of hard-coded colors.
  float sunDisc = max(dot(env.sunDir, viewDir), 0.0);
  float sunGlow = pow(sunDisc, 5.0);
  float horizonGlow = 1.0-smoothstep(0.02, 0.72, abs(viewDir.y));
  float dawnGlow = dawnShape*horizonGlow;
  vec3 dawnGlowCol = mix(skyCol.horizon, skyCol.horizonEdge, 0.35);
  sky += dawnGlowCol*(0.53*dawnGlow + 2.6*sunGlow*dawnShape);

  // Warm haze is strongest close to the horizon and fades smoothly into the
  // configured dawn zenith. This is what produces the orange/yellow base seen
  // in the reference without changing NL_DAWN_HORIZON_COL in the config.
  float haze = dawnShape*(1.0-smoothstep(-0.12, 0.50, viewDir.y));
  sky += skyCol.horizon*(0.18*haze);

  sky *= 0.5+0.5*gradient2;
  sky *= (1.0 + (2.0*mg8 + 7.0*mg8*mg8)*mask)*mix(1.0, mask, NL_SKY_VOID_DARKNESS);

  if (!isSkyPlane) {
    float source = max(0.0, (mg8-0.25)/0.82);
    source *= source;
    source *= source;
    sky *= 1.0 + 16.0*source*(1.0-env.rainFactor);
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
  float time = t*0.12;
  float y = clamp(viewDir.y*0.5+0.5,0.0,1.0);
  float grad = 0.5+0.5*viewDir.y;
  float verticalCloud = 1.0-grad*grad*grad;

  float horizon = smoothstep(-0.20,0.08,viewDir.y);

  float upper = smoothstep(0.0,1.0,y);
  upper *= upper;

  vec3 sky = mix(horizonCol,zenithCol,upper);

  float a = atan2(viewDir.x,viewDir.z);

  float wave1 = sin(3.0*a+time*1.8+10.0*viewDir.x*viewDir.y);
  float wave2 = sin(5.0*a-time*1.15+5.0*wave1+0.1*sin(40.0*a-4.0*time));
  float wave3 = sin(12.0*a+time*0.65+15.0*viewDir.x*viewDir.y);

  float cosmic = wave1*0.45+wave2*0.35+wave3*0.20;
  cosmic = cosmic*0.5+0.5;
  cosmic = smoothstep(0.18,0.86,cosmic);

  vec3 flowPos1 = vec3(viewDir.x*2.8+time*0.42+10.0*viewDir.x*viewDir.y*0.10, viewDir.y*3.2+sin(time*0.65)*0.16, viewDir.z*2.8);
  vec3 flowPos2 = vec3(viewDir.x*7.0-time*0.28+5.0*viewDir.x*viewDir.y*0.08, viewDir.y*6.0-time*0.10, viewDir.z*7.0);
  vec3 flowPos3 = vec3(viewDir.x*17.0+time*0.18+15.0*viewDir.x*viewDir.y*0.05, viewDir.y*13.0, viewDir.z*17.0-time*0.24);

  float n1 = noise3D(flowPos1);
  float n2 = noise3D(flowPos2);
  float n3 = noise3D(flowPos3);

  float nebula = n1*0.50+n2*0.32+n3*0.18;
  nebula = smoothstep(0.27,0.73,nebula);
  nebula *= 0.20+0.80*verticalCloud;
  nebula *= 0.72+0.28*cosmic;

  vec3 cyanNebula = vec3(0.003,0.22,0.31);
  cyanNebula *= nebula*1.75;

  float violetPattern = sin(4.0*a-viewDir.y*8.0+time*1.35);
  violetPattern = violetPattern*0.5+0.5;
  violetPattern *= nebula;
  violetPattern = smoothstep(0.20,0.82,violetPattern);

  vec3 violetNebula = vec3(0.17,0.008,0.36);
  violetNebula *= violetPattern*1.70;

  float magentaPattern = sin(7.0*a+viewDir.y*12.0-time*0.85+n2*3.0);
  magentaPattern = magentaPattern*0.5+0.5;
  magentaPattern *= nebula;
  magentaPattern = smoothstep(0.38,0.88,magentaPattern);

  vec3 magentaNebula = vec3(0.20,0.008,0.25);
  magentaNebula *= magentaPattern*1.05;

  float center = verticalCloud;
  center = smoothstep(0.02,0.94,center);
  center *= 0.30+0.70*nebula;

  float pulse = 0.5+0.5*sin(time*2.5);
  center *= 0.76+0.24*pulse;

  vec3 centerGlow = vec3(0.008,0.20,0.29);
  centerGlow *= center*0.72;

  float aura = smoothstep(0.02,0.72,verticalCloud);
  aura *= 0.35+0.65*nebula;

  vec3 royalAura = vec3(0.075,0.004,0.17);
  royalAura *= aura*0.80;

  float filament = abs(sin(17.0*a+24.0*viewDir.y+n1*8.0-time*2.4));
  filament = 1.0-filament;
  filament = smoothstep(0.82,0.997,filament);
  filament *= nebula*verticalCloud;

  vec3 filamentColor = vec3(0.012,0.29,0.38);
  filamentColor *= filament*0.82;

  float violetFilament = abs(sin(10.0*a-21.0*viewDir.y+n2*7.0+time*1.3));
  violetFilament = 1.0-violetFilament;
  violetFilament = smoothstep(0.84,0.997,violetFilament);
  violetFilament *= nebula*verticalCloud;

  vec3 violetFilamentColor = vec3(0.11,0.004,0.25);
  violetFilamentColor *= violetFilament*0.72;

  sky += cyanNebula;
  sky += violetNebula;
  sky += magentaNebula;
  sky += centerGlow;
  sky += royalAura;
  sky += filamentColor;
  sky += violetFilamentColor;

  float stormTime = time*0.50;

  vec3 stormPos1 = vec3(viewDir.x*4.0+stormTime*0.16, viewDir.y*3.5-stormTime*0.07, viewDir.z*4.0);
  vec3 stormPos2 = vec3(viewDir.x*9.0-stormTime*0.11, viewDir.y*7.0+stormTime*0.05, viewDir.z*9.0);

  float storm1 = noise3D(stormPos1);
  float storm2 = noise3D(stormPos2);

  float stormCloud = storm1*0.65+storm2*0.35;
  stormCloud = smoothstep(0.48,0.76,stormCloud);
  stormCloud *= verticalCloud;

  float strikeNoise = noise3D(vec3(viewDir.x*12.0+time*0.38+10.0*viewDir.x*viewDir.y*0.05, viewDir.y*10.0-time*0.27, viewDir.z*12.0+time*0.31));

  float strike = smoothstep(0.91,0.985,strikeNoise);
  strike *= stormCloud;

  float bolt = abs(sin(9.0*a+19.0*viewDir.y+storm1*9.0));
  bolt = 1.0-bolt;
  bolt = smoothstep(0.972,0.999,bolt);

  float branch1 = abs(sin(17.0*a-31.0*viewDir.y+storm2*13.0+storm1*5.0));
  branch1 = 1.0-branch1;
  branch1 = smoothstep(0.978,0.999,branch1);

  float branch2 = abs(sin(27.0*a+42.0*viewDir.y+storm1*17.0));
  branch2 = 1.0-branch2;
  branch2 = smoothstep(0.984,0.999,branch2);

  float lightning = bolt*0.90+branch1*0.65+branch2*0.38;
  lightning = clamp(lightning,0.0,1.0);
  lightning *= strike;
  lightning *= smoothstep(0.08,0.88,verticalCloud);

  float flash = strike*stormCloud;
  flash *= horizon;

  vec3 thunderFlash = vec3(0.004,0.075,0.13);
  thunderFlash *= flash*1.20;

  vec3 lightningGlow = vec3(0.004,0.20,0.30);
  lightningGlow *= lightning*2.35;

  vec3 violetLightning = vec3(0.065,0.004,0.19);
  violetLightning *= lightning*1.05;

  vec3 lightningCore = vec3(0.78,0.96,1.0);
  lightningCore *= lightning*3.55;

  sky += thunderFlash;
  sky += lightningGlow;
  sky += violetLightning;
  sky += lightningCore;

  float voidMask = smoothstep(-0.25,0.18,viewDir.y);

  float lon = atan2(viewDir.x,viewDir.z)/(2.0*PI)+0.5;
  float lat = asin(clamp(viewDir.y,-1.0,1.0))/PI+0.5;

  vec2 starGrid = vec2(205.0,102.0);
  vec2 starPos = vec2(lon,lat)*starGrid;
  vec2 starCell = floor(starPos);
  vec2 starLocal = fract(starPos);

  float starSeed = rand(starCell+vec2(17.13,41.71));
  float starJitterX = rand(starCell+vec2(83.21,11.37));
  float starJitterY = rand(starCell+vec2(29.47,67.83));

  vec2 starCenter = 0.18+0.64*vec2(starJitterX,starJitterY);

  float starDistance = length((starLocal-starCenter)*vec2(1.0,1.12));

  float starRadius = 0.027;
  float starPoint = smoothstep(starRadius,starRadius*0.16,starDistance);
  starPoint *= step(0.935,starSeed);

  float brightSeed = rand(starCell+vec2(101.7,37.4));
  float brightRadius = 0.030;
  float brightPoint = smoothstep(brightRadius,brightRadius*0.15,starDistance);
  brightPoint *= step(0.981,brightSeed);

  float starGlow = smoothstep(0.075,0.012,starDistance);
  starGlow *= step(0.935,starSeed);

  float starMask = 0.92+0.08*voidMask;
  starMask *= smoothstep(-0.02,0.08,viewDir.y);

  float starBrightness = starPoint*1.18+brightPoint*1.75;

  vec3 starColor = mix(vec3(0.72,0.84,1.0),vec3(1.0,0.91,0.76),brightSeed);

  sky += starColor*starBrightness*starMask;
  sky += starColor*starGlow*0.08*starMask;

  sky *= 0.70+0.30*voidMask;
  sky = max(sky,vec3(0.0,0.0,0.0));
  sky *= 1.10;

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
       if (env.underwater) {
         float a = atan2(viewDir.x, viewDir.z);
         float grad = 0.5 + 0.5*viewDir.y;
         grad *= grad;
         float spread = (0.5 + 0.5*sin(3.0*a + 0.2*t + 2.0*sin(5.0*a - 0.4*t)));
         spread *= (0.5 + 0.5*sin(3.0*a - sin(0.5*t)))*grad;
         spread += (1.0-spread)*grad;
         float streaks = spread*spread;
         streaks *= streaks;
         streaks = (spread + 3.0*grad*grad + 4.0*streaks*streaks);
         sky += 2.0*streaks*skycol.horizon;
       }
    #endif
  }

  return sky;
}

// shooting star
vec3 nlRenderShootingStar(vec3 viewDir, vec3 FOG_COLOR, float t) {
  // transition vars
  float h = t / (NL_SHOOTING_STAR_DELAY + NL_SHOOTING_STAR_PERIOD);
  float h0 = floor(h);
  t = (NL_SHOOTING_STAR_DELAY + NL_SHOOTING_STAR_PERIOD) * (h-h0);
  t = min(t/NL_SHOOTING_STAR_PERIOD, 1.0);
  float t0 = t*t;
  float t1 = 1.0-t0;
  t1 *= t1; t1 *= t1; t1 *= t1;

  // randomize size, rotation, add motion, add skew
  float r = fract(sin(h0) * 43758.545313);
  float a = 6.2831*r;
  float cosa = cos(a);
  float sina = sin(a);
  vec2 uv = viewDir.xz * (6.0 + 4.0*r);
  uv = vec2(cosa*uv.x + sina*uv.y, -sina*uv.x + cosa*uv.y);
  uv.x += t1 - t;
  uv.x -= 2.0*r + 3.5;
  uv.y += viewDir.y * 3.0;

  // draw star
  float g = 1.0-min(abs((uv.x-0.95))*20.0, 1.0); // source glow
  float s = 1.0-min(abs(8.0*uv.y), 1.0); // line
  s *= s*s*smoothstep(-1.0+1.96*t1, 0.98-t, uv.x); // decay tail
  s *= s*s*smoothstep(1.0, 0.98-t0, uv.x); // decay source
  s *= 1.0-t1; // fade in
  s *= 1.0-t0; // fade out
  s *= 0.7 + 16.0*g*g;
  s *= max(1.0-FOG_COLOR.r-FOG_COLOR.g-FOG_COLOR.b, 0.0); // fade out during day
  return s*vec3(0.8, 0.9, 1.0);
}

// Galaxy stars - needs further optimization
vec3 nlRenderGalaxy(vec3 vdir, vec3 fogColor, nl_environment env, float t) {
  if (env.underwater) {
    return vec3_splat(0.0);
  }

  t *= NL_GALAXY_SPEED;

  // rotate space
  float cosb = sin(0.2*t);
  float sinb = cos(0.2*t);
  vdir.xy = mul(mat2(cosb, sinb, -sinb, cosb), vdir.xy);

  // noise
  float n0 = 0.5 + 0.5*sin(5.0*vdir.x)*sin(5.0*vdir.y - 0.5*t)*sin(5.0*vdir.z + 0.5*t);
  float n1 = noise3D(15.0*vdir + sin(0.85*t + 1.3));
  float n2 = noise3D(50.0*vdir + 1.0*n1 + sin(0.7*t + 1.0));
  float n3 = noise3D(200.0*vdir - 10.0*sin(0.4*t + 0.500));

  // stars
  n3 = smoothstep(0.04,0.3,n3+0.02*n2);
  float gd = vdir.x + 0.1*vdir.y + 0.1*sin(10.0*vdir.z + 0.2*t);
  float st = n1*n2*n3*n3*(1.0+70.0*gd*gd);
  st = (1.0-st)/(1.0+200.0*st);
  vec3 stars = (0.8 + 0.2*sin(vec3(8.0,6.0,10.0)*(2.0*n1+0.8*n2) + vec3(0.0,0.4,0.82)))*st;

  // glow
  float gfmask = abs(vdir.x)-0.15*n1+0.04*n2+0.25*n0;
  float gf = 1.0 - (vdir.x*vdir.x + 0.03*n1 + 0.2*n0);
  gf *= gf;
  gf *= gf*gf;
  gf *= 1.0-0.3*smoothstep(0.2, 0.3, gfmask);
  gf *= 1.0-0.2*smoothstep(0.3, 0.4, gfmask);
  gf *= 1.0-0.1*smoothstep(0.2, 0.1, gfmask);
  vec3 gfcol = normalize(vec3(n0, cos(2.0*vdir.y), sin(vdir.x+n0)));
  stars += (0.4*gf + 0.012)*mix(vec3(0.5, 0.5, 0.5), gfcol*gfcol, NL_GALAXY_VIBRANCE);

  stars *= mix(1.0, NL_GALAXY_DAY_VISIBILITY, nlDayLightFactor(env.dayFactor));

  return stars*(1.0-env.rainFactor);
}


#endif
