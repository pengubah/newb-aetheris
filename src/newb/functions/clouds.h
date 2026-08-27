#ifndef CLOUDS_H
#define CLOUDS_H

#include "detection.h"
#include "noise.h"
#include "sky.h"

// simple clouds 2D noise
float cloudNoise2D(vec2 p, highp float t, float rain) {
  t *= NL_CLOUD1_SPEED;
  p += t;
  p.y += 3.0*sin(0.3*p.x + 0.1*t);

  vec2 p0 = floor(p);
  vec2 u = p-p0;
  u *= u*(3.0-2.0*u);
  vec2 v = 1.0-u;

  float n = mix(
    mix(rand(p0),rand(p0+vec2(1.0,0.0)), u.x),
    mix(rand(p0+vec2(0.0,1.0)),rand(p0+vec2(1.0,1.0)), u.x),
    u.y
  );
  n *= 0.5 + 0.5*sin(p.x*0.6 - 0.5*t)*sin(p.y*0.6 + 0.8*t);
  n = min(n*(1.0+rain), 1.0);
  return n*n;
}

// simple clouds
vec4 renderCloudsSimple(nl_skycolor skycol, vec3 pos, highp float t, float rain) {
  pos.xz *= NL_CLOUD1_SCALE;
  float d = cloudNoise2D(pos.xz, t, rain);
  vec4 col = vec4(skycol.horizonEdge + skycol.zenith, smoothstep(0.1,0.6,d));
  col.rgb += 1.5*dot(col.rgb, vec3(0.3,0.4,0.3))*smoothstep(0.6,0.2,d)*col.a;
  col.rgb *= 1.0 - 0.8*rain;
  return col;
}

// rounded clouds

// rounded clouds 3D density map
float cloudDf(vec3 pos, float rain, vec2 boxiness) {
  boxiness *= 0.999;
  vec2 p0 = floor(pos.xz);
  vec2 u = max((pos.xz-p0-boxiness.x)/(1.0-boxiness.x), 0.0);
  u *= u*(3.0 - 2.0*u);

  vec4 r = vec4(rand(p0), rand(p0+vec2(1.0,0.0)), rand(p0+vec2(1.0,1.0)), rand(p0+vec2(0.0,1.0)));
  r = smoothstep(0.1001+0.2*rain, 0.1+0.2*rain*rain, r); // rain transition

  float n = mix(mix(r.x,r.y,u.x), mix(r.w,r.z,u.x), u.y);

  // round y
  n *= 1.0 - 1.5*smoothstep(boxiness.y, 2.0 - boxiness.y, 2.0*abs(pos.y-0.5));

  n = max(1.25*(n-0.2), 0.0); // smoothstep(0.2, 1.0, n)
  n *= n*(3.0 - 2.0*n);
  return n;
}

vec4 renderCloudsRounded(
    vec3 vDir, vec3 vPos, float rain, float time, vec3 horizonCol, vec3 zenithCol, const float thickness, const float thickness_rain, const float speed,
    const vec2 scale, const float density, const vec2 boxiness
) {
  float height = 9.0*mix(thickness, thickness_rain, rain);
  float stepsf = 5.0;

  // scaled ray offset
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = height*scale*vDir.xz/(0.02+0.98*abs(vDir.y));

  // local cloud pos
  vec3 pos;
  pos.y = 0.0;
  pos.xz = scale*(vPos.xz + vec2(1.0,0.5)*(time*speed));
  pos += deltaP;

  deltaP /= -stepsf;
  pos += deltaP * hash(vPos.xz + time); // Displace Clouds' Step

  // alpha, gradient
  vec2 d = vec2(0.0,0.5);
  for (int i=1; i<=int(stepsf); i++) {
    float m = cloudDf(pos, rain, boxiness);
    d.x += m;
    d.y = mix(d.y, pos.y, m);
    pos += deltaP;
  }
  d.x *= smoothstep(0.7, 1.0, d.x);
  d.x /= (stepsf/density) + d.x;

  if (vPos.y < 0.0) { // view from top
    d.y = 1.0 - d.y;
  }

  vec4 col = vec4(horizonCol + zenithCol, d.x);
  col.rgb = mix(col.rgb, mix(col.rgb,zenithCol,1.0), smoothstep(1.0,0.1,d.y));
  col.rgb += dot(col.rgb, vec3(0.3,0.4,0.3))*d.y*d.y;
  col.rgb *= 1.0 - 0.8*rain;
  return col;
}

float cloudsNoiseVr(vec2 p, float t) {
  float n = fastVoronoi2(p + t, 1.8);
  n *= fastVoronoi2(3.0*p + t, 1.5);
  n *= fastVoronoi2(9.0*p + t, 0.4);
  n *= fastVoronoi2(27.0*p + t, 0.1);
  //n *= fastVoronoi2(82.0*pos + t, 0.02); // more quality
  return n*n;
}

vec4 renderClouds(vec2 p, float t, float rain, vec3 horizonCol, vec3 zenithCol, const vec2 scale, const float velocity, const float shadow) {
  #if NL_CLOUD_TYPE == 4
    return renderCloudsPixelated(p, t, rain, horizonCol, zenithCol, scale, velocity, shadow);
  #endif
  p *= scale;
  t *= velocity;

  // layer 1
  float a = cloudsNoiseVr(p, t);
  float b = cloudsNoiseVr(p + NL_CLOUD3_SHADOW_OFFSET*scale, t);

  // layer 2
  p = 1.4 * p.yx + vec2(7.8, 9.2);
  t *= 0.5;
  float c = cloudsNoiseVr(p, t);
  float d = cloudsNoiseVr(p + NL_CLOUD3_SHADOW_OFFSET*scale, t);

  // higher = less clouds thickness
  // lower separation betwen x & y = sharper
  vec2 tr = vec2(0.6, 0.7) - 0.12*rain;
  a = smoothstep(tr.x, tr.y, a);
  c = smoothstep(tr.x, tr.y, c);

  // shadow
  b *= smoothstep(0.2, 0.8, b);
  d *= smoothstep(0.2, 0.8, d);

  vec4 col;
  col.a = a + c*(1.0-a);
  col.rgb = horizonCol + horizonCol.ggg;
  col.rgb = mix(col.rgb, 0.5*(zenithCol + zenithCol.ggg), shadow*mix(b, d, c));
  col.rgb *= 1.0-0.7*rain;

  return col;
}



// pixelated clouds
// Adapted from the standalone pixelated-cloud shader into the Newb/Lazurite cloud pipeline.
// NL_CLOUD_TYPE == 4 selects this cloud style.
#ifndef NL_CLOUD4_SCALE
#define NL_CLOUD4_SCALE 0.075
#endif
#ifndef NL_CLOUD4_SPEED
#define NL_CLOUD4_SPEED 0.012
#endif
#ifndef NL_CLOUD4_THRESHOLD
#define NL_CLOUD4_THRESHOLD 0.725
#endif
#ifndef NL_CLOUD4_CELL_SIZE
#define NL_CLOUD4_CELL_SIZE 0.60
#endif
#ifndef NL_CLOUD4_CLUSTER_SIZE
#define NL_CLOUD4_CLUSTER_SIZE 4.93
#endif
float cloudPixelHash(vec2 p) {
  return fract(cos(p.x + p.y*332.0)*335.552);
}

float cloudPixelNoise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  f = f*f*(3.0-2.0*f);

  float a = cloudPixelHash(i);
  float b = cloudPixelHash(i+vec2(1.0,0.0));
  float c = cloudPixelHash(i+vec2(0.0,1.0));
  float d = cloudPixelHash(i+vec2(1.0,1.0));

  return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
}

float cloudPixelCell(vec2 uv, float size) {
  vec2 f = fract(uv)-0.5;
  float d = max(abs(f.x),abs(f.y))-size;
  return smoothstep(0.03,-0.03,d);
}

// Returns: x = cloud coverage, y = cloud body, z = pixelated rim.
vec3 cloudPixelDensity(vec2 worldXZ, highp float t, float rain) {
  vec2 uv = worldXZ*NL_CLOUD4_SCALE;
  float cloudTime = -t*NL_CLOUD4_SPEED;
  float clusterSize = NL_CLOUD4_CLUSTER_SIZE;
  float threshold = mix(NL_CLOUD4_THRESHOLD, NL_CLOUD4_THRESHOLD-0.08, rain);

  float body = 0.0;
  float shade = 0.0;

  for (int i=0; i<3; i++) {
    uv /= 1.007;
    float density = cloudPixelNoise(floor(uv+cloudTime)/clusterSize);
    float c = step(threshold,density);
    float cellMask = cloudPixelCell(uv+cloudTime,NL_CLOUD4_CELL_SIZE);
    body = max(body,cellMask*c);
  }

  float shadeDensity = cloudPixelNoise(floor(uv+cloudTime)/clusterSize);
  float shadeCell = step(threshold,shadeDensity);
  float shadeShape = cloudPixelCell(uv+cloudTime,NL_CLOUD4_CELL_SIZE);
  shade = shadeCell*shadeShape;

  // Pixel-cell rim. Kept cheap: four neighboring samples.
  float rim = 0.0;
  vec2 e = vec2(0.15,0.0);
  vec2 q = uv+cloudTime;

  float n1 = step(threshold,cloudPixelNoise(floor(q+vec2(e.x,0.0))/clusterSize))*cloudPixelCell(q+vec2(e.x,0.0),NL_CLOUD4_CELL_SIZE);
  float n2 = step(threshold,cloudPixelNoise(floor(q-vec2(e.x,0.0))/clusterSize))*cloudPixelCell(q-vec2(e.x,0.0),NL_CLOUD4_CELL_SIZE);
  float n3 = step(threshold,cloudPixelNoise(floor(q+vec2(0.0,e.x))/clusterSize))*cloudPixelCell(q+vec2(0.0,e.x),NL_CLOUD4_CELL_SIZE);
  float n4 = step(threshold,cloudPixelNoise(floor(q-vec2(0.0,e.x))/clusterSize))*cloudPixelCell(q-vec2(0.0,e.x),NL_CLOUD4_CELL_SIZE);

  rim = shade*(1.0-n1);
  rim += shade*(1.0-n2);
  rim += shade*(1.0-n3);
  rim += shade*(1.0-n4);
  rim = clamp(rim,0.0,1.0);

  body -= shade*body*0.20;
  body = clamp(body,0.0,1.0);

  // Rain makes the cloud layer flatter and slightly denser.
  body = mix(body,body*0.88+0.12*shade,rain*0.35);

  return vec3(body,shade,rim);
}

vec4 renderCloudsPixelated(
    vec2 p, highp float t, float rain,
    vec3 horizonCol, vec3 zenithCol,
    const vec2 scale, const float velocity, const float shadow
) {
  vec2 cloudPos = p;
  float localTime = t*velocity;
  vec3 cloud = cloudPixelDensity(cloudPos,localTime,rain);

  // Softer fade near the horizon prevents a hard rectangular cloud sheet.
  float horizonFade = smoothstep(-0.15,0.18,p.y);
  float alpha = cloud.x*horizonFade;

  // Keep the original sky palette, but brighten the cloud top instead of using
  // an overexposed vec3(1.5+) like the standalone preview shader.
  vec3 baseCol = mix(horizonCol,zenithCol,0.72);
  vec3 cloudTop = mix(baseCol,vec3_splat(1.0),0.68);
  vec3 cloudBottom = mix(baseCol,0.55*baseCol,0.32);
  vec3 col = mix(cloudBottom,cloudTop,0.55+0.45*cloud.y);

  // Subtle pixel rim and shadow for depth.
  col += 0.12*cloud.z*mix(1.0,0.65,rain)*shadow;
  col *= 1.0-0.28*rain;

  return vec4(col,alpha);
}

// aurora is rendered on clouds layer
#ifdef NL_AURORA
vec4 renderAurora(vec3 p, float t, float rain, vec3 FOG_COLOR) {
  t *= NL_AURORA_VELOCITY;
  p.xz *= NL_AURORA_SCALE;
  p.xz += 0.05*sin(p.x*4.0 + 20.0*t);

  float d0 = sin(p.x*0.1 + t + sin(p.z*0.2));
  float d1 = sin(p.z*0.1 - t + sin(p.x*0.2));
  float d2 = sin(p.z*0.1 + 1.0*sin(d0 + d1*2.0) + d1*2.0 + d0*1.0);
  d0 *= d0; d1 *= d1; d2 *= d2;
  d2 = d0/(1.0 + d2/NL_AURORA_WIDTH);

  float mask = (1.0-0.8*rain)*max(1.0 - 4.0*max(FOG_COLOR.b, FOG_COLOR.g), 0.0);
  return vec4(NL_AURORA*mix(NL_AURORA_COL1,NL_AURORA_COL2,d1),1.0)*d2*mask;
}
#endif

vec4 nlCloudAuroraReflection(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 wPos, vec3 CAMERA_POS, highp float t) {
  vec2 cloudPos = wPos.xz;
  cloudPos += (187.0-(wPos.y+CAMERA_POS.y))*viewDir.xz/viewDir.y;
  float fade = clamp(2.0 - 0.005*length(cloudPos), 0.0, 1.0);
  cloudPos += CAMERA_POS.xz;

  vec4 refl = vec4_splat(0.0);

  #ifdef NL_AURORA
    vec4 aurora = renderAurora(cloudPos.xyy, t, env.rainFactor, env.fogCol);
    aurora.a *= fade;
    refl = vec4(2.0*aurora.rgb*aurora.a, aurora.a);
  #endif

  #if NL_CLOUD_TYPE == 1
    vec4 clouds = renderCloudsSimple(skycol, cloudPos.xyy, t, env.rainFactor);
    clouds.a *= fade;
    refl = vec4(mix(refl.rgb, clouds.rgb, clouds.a), min(refl.a + clouds.a, 1.0));
  #elif NL_CLOUD_TYPE == 4
    vec4 clouds = renderCloudsPixelated(cloudPos, t, env.rainFactor, skycol.horizon, skycol.zenith, vec2(NL_CLOUD4_SCALE, NL_CLOUD4_SCALE), 1.0, 1.0);
    clouds.a *= fade;
    refl = vec4(mix(refl.rgb, clouds.rgb, clouds.a), min(refl.a + clouds.a, 1.0));
  #endif

  return refl;
}

#endif
