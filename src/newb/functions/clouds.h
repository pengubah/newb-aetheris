#ifndef CLOUDS_H
#define CLOUDS_H

#include "detection.h"
#include "noise.h"
#include "sky.h"

float cloudNoise2D(vec2 p, highp float t, float rain) {
  t *= NL_CLOUD1_SPEED;
  p += vec2(t,0.22*sin(t));
  p.y += 3.0*sin(0.3*p.x+0.1*t);
  vec2 p0 = floor(p);
  vec2 u = fract(p);
  u *= u*(3.0-2.0*u);
  float n = mix(mix(rand(p0),rand(p0+vec2(1.0,0.0)),u.x),mix(rand(p0+vec2(0.0,1.0)),rand(p0+vec2(1.0,1.0)),u.x),u.y);
  float detail = 0.5+0.5*sin(p.x*0.6-0.5*t)*sin(p.y*0.6+0.8*t);
  n = mix(n,0.72*n+0.28*n*detail,0.55);
  n = min(n*(1.0+rain*0.85),1.0);
  return n*n;
}

vec4 renderCloudsSimple(nl_skycolor skycol, vec3 pos, highp float t, float rain) {
  pos.xz *= NL_CLOUD1_SCALE;
  float d = cloudNoise2D(pos.xz,t,rain);
  float a = smoothstep(0.12,0.64,d);
  vec3 base = mix(skycol.horizonEdge,skycol.zenith,0.35+0.35*smoothstep(-1.0,1.0,pos.y));
  vec4 col = vec4(base,a);
  float highlight = pow(max(dot(base,vec3_splat(0.333333)),0.0),0.45);
  col.rgb += base*highlight*(0.18+0.22*a);
  col.rgb *= 1.0-0.72*rain;
  return col;
}

float cloudDf(vec3 pos, float rain, vec2 boxiness) {
  boxiness *= 0.999;
  vec2 p0 = floor(pos.xz);
  vec2 u = max((pos.xz-p0-boxiness.x)/(1.0-boxiness.x),0.0);
  u *= u*(3.0-2.0*u);
  vec4 r = vec4(rand(p0),rand(p0+vec2(1.0,0.0)),rand(p0+vec2(1.0,1.0)),rand(p0+vec2(0.0,1.0)));
  r = smoothstep(0.1001+0.2*rain,0.1+0.2*rain*rain,r);
  float n = mix(mix(r.x,r.y,u.x),mix(r.w,r.z,u.x),u.y);
  float vertical = 1.0-smoothstep(boxiness.y,2.0-boxiness.y,2.0*abs(pos.y-0.5));
  vertical *= 0.82+0.18*smoothstep(0.0,1.0,vertical);
  n *= vertical;
  n = max(1.25*(n-0.2),0.0);
  n *= n*(3.0-2.0*n);
  return n;
}

vec4 renderCloudsRounded(
    vec3 vDir, vec3 vPos, float rain, float time, vec3 horizonCol, vec3 zenithCol,
    const int steps, const float thickness, const float thickness_rain, const float speed,
    const vec2 scale, const float density, const vec2 boxiness
) {
  float height = 7.0*mix(thickness,thickness_rain,rain);
  float stepsf = float(steps);
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = height*scale*vDir.xz/(0.028+0.972*abs(vDir.y));
  vec3 pos;
  pos.y = 0.0;
  pos.xz = scale*(vPos.xz+vec2(1.0,0.5)*(time*speed));
  pos += deltaP;
  deltaP /= -stepsf;

  vec3 accum = vec3_splat(0.0);
  float alpha = 0.0;
  float weightedY = 0.0;
  for (int i=1;i<=steps;i++) {
    float m = cloudDf(pos,rain,boxiness);
    float edge = smoothstep(0.0,0.24,m);
    float lightTop = smoothstep(-0.25,0.7,pos.y);
    float local = m*(0.58+0.42*lightTop);
    accum += local*(horizonCol*0.78+zenithCol*0.22);
    alpha += m;
    weightedY += pos.y*m;
    pos += deltaP;
  }

  alpha *= smoothstep(0.025,0.14,alpha);
  alpha /= (stepsf/max(density,0.01))+alpha;
  float meanY = weightedY/max(alpha*stepsf,0.001);
  if (vPos.y<0.0) meanY = 1.0-meanY;

  vec3 col = accum/max(alpha*stepsf,0.001);
  vec3 cloudBase = mix(horizonCol,zenithCol,0.28+0.34*clamp(meanY,0.0,1.0));
  col = mix(cloudBase,col,0.72);
  float luma = dot(col,vec3_splat(0.333333));
  col += col*(0.10+0.16*meanY)*(0.55+0.45*luma);
  float silver = smoothstep(0.65,0.98,alpha)*(1.0-rain);
  col += horizonCol*silver*0.12;
  col *= 1.0-0.70*rain;
  return vec4(col,clamp(alpha,0.0,1.0));
}

float cloudsNoiseVr(vec2 p, float t) {
  float n = fastVoronoi2(p+t,1.8);
  n *= fastVoronoi2(3.0*p+t*0.84,1.5);
  n *= fastVoronoi2(9.0*p+t*0.62,0.4);
  n *= fastVoronoi2(27.0*p+t*0.44,0.1);
  return n*n;
}

vec4 renderClouds(vec2 p, float t, float rain, vec3 horizonCol, vec3 zenithCol, const vec2 scale, const float velocity, const float shadow) {
  p *= scale;
  t *= velocity;
  float a = cloudsNoiseVr(p,t);
  float b = cloudsNoiseVr(p+NL_CLOUD3_SHADOW_OFFSET*scale,t);
  p = 1.4*p.yx+vec2(7.8,9.2);
  t *= 0.5;
  float c = cloudsNoiseVr(p,t);
  float d = cloudsNoiseVr(p+NL_CLOUD3_SHADOW_OFFSET*scale,t);
  vec2 tr = vec2(0.56,0.72)-0.12*rain;
  a = smoothstep(tr.x,tr.y,a);
  c = smoothstep(tr.x,tr.y,c);
  b = smoothstep(0.18,0.82,b);
  d = smoothstep(0.18,0.82,d);
  float alpha = a+c*(1.0-a);
  float shadowMask = mix(b,d,c)*shadow;
  vec3 sunlit = horizonCol+horizonCol.ggg;
  vec3 shaded = 0.5*(zenithCol+zenithCol.ggg);
  vec3 col = mix(sunlit,shaded,shadowMask*0.72);
  float rim = smoothstep(0.62,0.98,alpha);
  col += horizonCol*rim*(0.10+0.12*(1.0-rain));
  col *= 1.0-0.68*rain;
  return vec4(col,alpha);
}

#ifdef NL_AURORA
vec4 renderAurora(vec3 p, float t, float rain, vec3 FOG_COLOR) {
  t *= NL_AURORA_VELOCITY;
  p.xz *= NL_AURORA_SCALE;
  p.xz += 0.05*sin(p.x*4.0+20.0*t);
  float d0 = sin(p.x*0.1+t+sin(p.z*0.2));
  float d1 = sin(p.z*0.1-t+sin(p.x*0.2));
  float d2 = sin(p.z*0.1+1.0*sin(d0+d1*2.0)+d1*2.0+d0);
  d0 *= d0; d1 *= d1; d2 *= d2;
  d2 = d0/(1.0+d2/NL_AURORA_WIDTH);
  float fogLum = dot(FOG_COLOR,vec3_splat(0.333333));
  float mask = (1.0-0.8*rain)*smoothstep(0.08,0.5,1.0-fogLum);
  return vec4(NL_AURORA*mix(NL_AURORA_COL1,NL_AURORA_COL2,d1),1.0)*d2*mask;
}
#endif

vec4 nlCloudAuroraReflection(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 wPos, vec3 CAMERA_POS, highp float t) {
  vec2 cloudPos = wPos.xz;
  cloudPos += (187.0-(wPos.y+CAMERA_POS.y))*viewDir.xz/max(viewDir.y,0.015);
  float fade = clamp(2.0-0.005*length(cloudPos),0.0,1.0);
  cloudPos += CAMERA_POS.xz;
  vec4 refl = vec4_splat(0.0);
  #ifdef NL_AURORA
    vec4 aurora = renderAurora(cloudPos.xyy,t,env.rainFactor,env.fogCol);
    aurora.a *= fade;
    refl = vec4(2.0*aurora.rgb*aurora.a,aurora.a);
  #endif
  #if NL_CLOUD_TYPE == 1
    vec4 clouds = renderCloudsSimple(skycol,cloudPos.xyy,t,env.rainFactor);
    clouds.a *= fade;
    refl = vec4(mix(refl.rgb,clouds.rgb,clouds.a),min(refl.a+clouds.a,1.0));
  #endif
  return refl;
}

#endif
