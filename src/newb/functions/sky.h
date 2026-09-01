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
  float df = mix(1.0, g2.x, dawnFactor*dawnFactor);
  vec3 sky = mix(skyCol.horizon, skyCol.horizonEdge, gradient1*df*df);
  sky = mix(skyCol.zenith, sky, gradient2*df);
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
    vr.xy = mat2(cos(r),-sin(r),sin(r),cos(r)) * vr.xy;
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
    float spiralWave = 0.5 + 0.5 * sin(spiralPhase);
    float spiralShape = pow(spiralWave,2.6);
    float innerSpiral = smoothstep(0.18,0.62,d) * spiralShape;
    float innerBreakup = 0.72 + 0.28 * spiralShape;
    float detail = 0.96 + 0.04 * sin(12.0 * a - 15.0 * d + t * 1.5);
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

vec3 renderEndSky(vec3 horizonCol, vec3 zenithCol, vec3 viewDir, float t) {
	t *= 0.015;
	float a = atan2(viewDir.x, viewDir.z);
	float y = viewDir.y;

	// Base sky
	float upper = smoothstep(-0.8, 0.95, y);
	vec3 sky = mix(vec3(0.0025, 0.0006, 0.008), vec3(0.009, 0.0015, 0.028), upper);
	sky += vec3(0.006, 0.001, 0.018)*smoothstep(-0.7, 0.8, y);

	// Main nebula coordinates
	vec2 nUV = vec2(a/3.14159265, y)*1.18;
	nUV.x += t*0.3;
	nUV.y += sin(nUV.x*1.6+t*0.35)*0.07;

	// Large cloud
	float n1 = endFbm(nUV*1.05);
	float n2 = endFbm(nUV*2.0+vec2(8.3, -4.1));
	float n3 = endFbm(nUV*3.8+vec2(-6.2, 13.7));

	float cloud = n1*0.56+n2*0.29+n3*0.15;
	cloud = smoothstep(0.33, 0.67, cloud);

	// Broad nebula
	float center = -0.02+sin(nUV.x*1.35+t*0.2)*0.15;
	float band = exp(-pow((y-center)*2.05, 2.0));
	cloud *= band;

	// Organic breakup
	float breakup = endFbm(nUV*5.0+vec2(t*0.45, -t*0.22));
	cloud *= 0.48+breakup*0.95;

	// Deep voids
	float voidNoise = endFbm(nUV*2.6+vec2(21.3, 6.7));
	float voids = smoothstep(0.48, 0.7, voidNoise);
	cloud *= 1.0-voids*0.64;

	// Secondary cloud
	vec2 farUV = vec2(a/3.14159265, y)*0.72+vec2(t*0.12, -0.04);
	float farCloud = endFbm(farUV*1.8+vec2(14.2, 7.3));
	farCloud = smoothstep(0.48, 0.73, farCloud);
	farCloud *= smoothstep(-0.65, 0.35, y);
	farCloud *= 0.42;

	sky += vec3(0.035, 0.002, 0.085)*farCloud;

	// Purple gas
	float purpleMix = smoothstep(0.34, 0.78, n2);
	vec3 purpleGas = mix(vec3(0.045, 0.0015, 0.13), vec3(0.27, 0.004, 0.34), purpleMix);

	// Magenta gas
	float magentaMix = smoothstep(0.57, 0.84, n3);
	purpleGas = mix(purpleGas, vec3(0.56, 0.008, 0.31), magentaMix*0.72);

	// Blue violet gas
	float blueMix = smoothstep(0.6, 0.88, n1);
	purpleGas = mix(purpleGas, vec3(0.018, 0.025, 0.22), blueMix*0.32);

	sky += purpleGas*cloud*1.82;

	// Bright gas
	float brightGas = endFbm(nUV*5.7+vec2(-t*0.35, t*0.18));
	brightGas = smoothstep(0.61, 0.82, brightGas);
	brightGas *= cloud;
	brightGas *= smoothstep(-0.7, 0.42, y);

	sky += vec3(0.68, 0.012, 0.39)*brightGas*0.95;

	// Violet emission
	float violetGas = endFbm(nUV*8.0+vec2(7.2, -12.1));
	violetGas = smoothstep(0.64, 0.84, violetGas);
	violetGas *= cloud;

	sky += vec3(0.3, 0.018, 0.7)*violetGas*0.62;

	// Fine cosmic dust
	float dust = endFbm(nUV*13.0+vec2(t*0.6, -t*0.32));
	dust = smoothstep(0.57, 0.79, dust);
	dust *= cloud;

	sky += vec3(0.32, 0.015, 0.3)*dust*0.32;

	// Dust lanes
	float laneNoise = endFbm(nUV*7.0+vec2(-t*0.15, t*0.08));
	float lanes = smoothstep(0.42, 0.58, laneNoise);
	lanes *= cloud;
	lanes = 1.0-lanes;
	lanes *= cloud;

	sky *= 1.0-lanes*0.28;

	// Blue gas
	float blueGas = endFbm(nUV*3.5+vec2(-16.0, 8.0));
	blueGas = smoothstep(0.54, 0.8, blueGas);
	blueGas *= cloud;

	sky += vec3(0.01, 0.04, 0.2)*blueGas*0.55;

	// Stars
	vec2 starUV = vec2(a/3.14159265, y)*108.0;
	vec2 starCell = floor(starUV);
	vec2 starLocal = fract(starUV)-0.5;

	float starRandom = endHash21(starCell);
	float starX = endHash21(starCell+vec2(17.3, 41.8))-0.5;
	float starY = endHash21(starCell+vec2(63.7, 9.2))-0.5;

	float starDistance = length(starLocal-vec2(starX, starY));
	float starSize = mix(0.007, 0.019, endHash21(starCell+vec2(4.7, 21.4)));
	float star = 1.0-smoothstep(starSize, starSize+0.006, starDistance);

	star *= step(0.74, starRandom);
	star *= smoothstep(-0.32, 0.16, y);

	// Star colors
	float starType = endHash21(starCell+vec2(71.4, 19.7));
	vec3 starColor = vec3(0.86, 0.9, 1.0);

	if (starType > 0.91) {
		starColor = vec3(1.0, 0.38, 0.22);
	} else if (starType > 0.8) {
		starColor = vec3(1.0, 0.7, 0.35);
	} else if (starType > 0.66) {
		starColor = vec3(0.4, 0.58, 1.0);
	} else if (starType > 0.5) {
		starColor = vec3(0.78, 0.42, 1.0);
	}

	sky += starColor*star*1.18;

	// Tiny stars
	vec2 tinyUV = vec2(a/3.14159265, y)*215.0;
	vec2 tinyCell = floor(tinyUV);
	vec2 tinyLocal = fract(tinyUV)-0.5;

	float tinyRandom = endHash21(tinyCell);
	float tinyX = endHash21(tinyCell+vec2(13.2, 37.6))-0.5;
	float tinyY = endHash21(tinyCell+vec2(52.4, 8.7))-0.5;

	float tinyDistance = length(tinyLocal-vec2(tinyX, tinyY));
	float tinyStar = 1.0-smoothstep(0.005, 0.014, tinyDistance);

	tinyStar *= step(0.84, tinyRandom);
	tinyStar *= smoothstep(-0.15, 0.2, y);

	float tinyType = endHash21(tinyCell+vec2(91.7, 24.2));
	vec3 tinyColor = mix(vec3(0.55, 0.63, 0.95), vec3(0.95, 0.52, 0.76), tinyType);

	sky += tinyColor*tinyStar*0.48;

	// Rare bright stars
	vec2 brightUV = vec2(a/3.14159265, y)*61.0;
	vec2 brightCell = floor(brightUV);
	vec2 brightLocal = fract(brightUV)-0.5;

	float brightRandom = endHash21(brightCell+vec2(31.7, 81.3));
	float brightX = endHash21(brightCell+vec2(14.1, 51.8))-0.5;
	float brightY = endHash21(brightCell+vec2(67.2, 8.1))-0.5;

	float brightDistance = length(brightLocal-vec2(brightX, brightY));
	float brightStar = 1.0-smoothstep(0.009, 0.026, brightDistance);

	brightStar *= step(0.969, brightRandom);
	brightStar *= smoothstep(-0.12, 0.22, y);

	float brightType = endHash21(brightCell+vec2(43.8, 17.4));
	vec3 brightColor = mix(vec3(0.82, 0.88, 1.0), vec3(1.0, 0.43, 0.55), brightType);

	sky += brightColor*brightStar*1.9;

	// Stellar glow
	float stellarGlow = endFbm(vec2(a/3.14159265, y)*9.0+vec2(31.0, 12.0));
	stellarGlow = smoothstep(0.78, 0.96, stellarGlow);
	stellarGlow *= star*0.18;

	sky += vec3(0.12, 0.08, 0.22)*stellarGlow;

	// Horizon atmosphere
	float horizonGlow = pow(max(0.0, 1.0-abs(y)), 9.0);
	sky += vec3(0.09, 0.002, 0.13)*horizonGlow*0.5;

	// Nebula bloom
	float bloom = smoothstep(0.42, 0.78, cloud);
	sky += vec3(0.055, 0.001, 0.09)*bloom*0.3;

	// Final tone
	sky = max(sky, vec3(0.0));
	sky = sky/(1.0+sky*0.18);
	sky = pow(sky, vec3(0.96));
	
	#ifdef NL_BLACKHOLE
      vec4 bh = renderBlackhole(viewdir,t);
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
  st = (1.0-st)/(1.0+400.0*st);
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

  stars *= mix(1.0, NL_GALAXY_DAY_VISIBILITY, env.dayFactor);

  return stars*(1.0-env.rainFactor);
}


#endif
