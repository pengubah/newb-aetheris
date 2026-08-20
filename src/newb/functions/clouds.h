#ifndef CLOUDS_H
#define CLOUDS_H

#include "detection.h"
#include "noise.h"
#include "sky.h"

float cloudNoise2D(vec2 p,highp float t,float rain) {
  t*=NL_CLOUD1_SPEED;
  p+=vec2(t,0.65*t);
  p.y+=3.0*sin(0.3*p.x+0.1*t);

  vec2 p0=floor(p);
  vec2 u=p-p0;
  u*=u*(3.0-2.0*u);
  vec2 v=1.0-u;

  float n=mix(
    mix(rand(p0),rand(p0+vec2(1.0,0.0)),u.x),
    mix(rand(p0+vec2(0.0,1.0)),rand(p0+vec2(1.0,1.0)),u.x),
    u.y
  );

  float detail=fbm2D(p*1.65+t*0.035);
  n=mix(n,detail,0.34);
  n*=0.5+0.5*sin(p.x*0.6-0.5*t)*sin(p.y*0.6+0.8*t);
  n=min(n*(1.0+rain),1.0);
  return n*n;
}

vec4 renderCloudsSimple(nl_skycolor skycol,vec3 pos,highp float t,float rain) {
  pos.xz*=NL_CLOUD1_SCALE;
  float d=cloudNoise2D(pos.xz,t,rain);
  float a=smoothstep(0.08,0.56,d);

  // Use only configured sky colors so clouds inherit dawn/day/night/rain identity.
  vec3 cloudLight=mix(skycol.horizon,skycol.zenith,0.55+0.45*smoothstep(-0.2,0.8,pos.y));
  float shade=0.72+0.28*smoothstep(0.15,0.9,d);
  vec3 col=cloudLight*shade;
  col+=pow(a,2.5)*0.24*mix(skycol.horizon,skycol.zenith,0.5)*(1.0-rain);
  col*=1.0-0.72*rain;
  return vec4(col,a*NL_CLOUD1_OPACITY);
}

float cloudDf(vec3 pos,float rain,vec2 boxiness) {
  boxiness*=0.999;
  vec2 p0=floor(pos.xz);
  vec2 u=max((pos.xz-p0-boxiness.x)/(1.0-boxiness.x),0.0);
  u*=u*(3.0-2.0*u);
  vec4 r=vec4(rand(p0),rand(p0+vec2(1.0,0.0)),rand(p0+vec2(1.0,1.0)),rand(p0+vec2(0.0,1.0)));
  r=smoothstep(0.1001+0.2*rain,0.1+0.2*rain*rain,r);
  float n=mix(mix(r.x,r.y,u.x),mix(r.w,r.z,u.x),u.y);
  n*=1.0-1.5*smoothstep(boxiness.y,2.0-boxiness.y,2.0*abs(pos.y-0.5));
  n=max(1.25*(n-0.2),0.0);
  n*=n*(3.0-2.0*n);
  return n;
}

vec4 renderCloudsRounded(vec3 vDir,vec3 vPos,float rain,float time,vec3 horizonCol,vec3 zenithCol,const int steps,const float thickness,const float thickness_rain,const float speed,const vec2 scale,const float density,const vec2 boxiness) {
  float height=7.0*mix(thickness,thickness_rain,rain);
  float stepsf=float(steps);

  vec3 deltaP;
  deltaP.y=1.0;
  deltaP.xz=height*scale*vDir.xz/(0.02+0.98*abs(vDir.y));

  vec3 pos;
  pos.y=0.0;
  pos.xz=scale*(vPos.xz+vec2(1.0,0.5)*(time*speed));
  pos+=deltaP;
  deltaP/=-stepsf;

  vec2 d=vec2(0.0,1.0);
  for(int i=1;i<=steps;i++) {
    float m=cloudDf(pos,rain,boxiness);
    // Small-scale breakup keeps the Newb rounded silhouette from becoming flat.
    float micro=0.82+0.18*fbm2D(pos.xz*2.5+time*0.012);
    m*=micro;
    d.x+=m;
    d.y=mix(d.y,pos.y,m);
    pos+=deltaP;
  }

  d.x*=smoothstep(0.025,0.10,d.x);
  d.x/=((stepsf/max(density,0.001))+d.x);
  d.x=smoothstep(0.03,0.92,d.x);

  if(vPos.y<0.0) d.y=1.0-d.y;

  // Stronger underside/upper-edge separation, but still derived entirely from sky config.
  float vertical=clamp(d.y,0.0,1.0);
  vec3 cloudTop=zenithCol*1.08;
  vec3 cloudBottom=horizonCol*0.86;
  vec3 col=mix(cloudBottom,cloudTop,smoothstep(0.08,0.92,vertical));
  float edge=smoothstep(0.12,0.72,d.x);
  col*=0.82+0.18*edge;
  col+=pow(d.x,3.0)*mix(horizonCol,zenithCol,0.58)*0.16*(1.0-rain);
  col*=1.0-0.68*rain;

  return vec4(col,d.x*NL_CLOUD2_DENSITY/40.0);
}

float cloudsNoiseVr(vec2 p,float t) {
  float n=fastVoronoi2(p+t,1.8);
  n*=fastVoronoi2(3.0*p+t,1.5);
  n*=fastVoronoi2(9.0*p+t,0.4);
  n*=fastVoronoi2(27.0*p+t,0.1);
  n*=0.92+0.08*fbm2D(p*1.7+t*0.04);
  return n*n;
}

vec4 renderClouds(vec2 p,float t,float rain,vec3 horizonCol,vec3 zenithCol,const vec2 scale,const float velocity,const float shadow) {
  p*=scale;
  t*=velocity;
  float a=cloudsNoiseVr(p,t);
  float b=cloudsNoiseVr(p+NL_CLOUD3_SHADOW_OFFSET*scale,t);
  p=1.4*p.yx+vec2(7.8,9.2);
  t*=0.5;
  float c=cloudsNoiseVr(p,t);
  float d=cloudsNoiseVr(p+NL_CLOUD3_SHADOW_OFFSET*scale,t);
  vec2 tr=vec2(0.56,0.72)-0.13*rain;
  a=smoothstep(tr.x,tr.y,a);
  c=smoothstep(tr.x,tr.y,c);
  b*=smoothstep(0.18,0.82,b);
  d*=smoothstep(0.18,0.82,d);
  float alpha=a+c*(1.0-a);
  vec3 col=mix(horizonCol*1.10,0.5*(zenithCol+zenithCol.ggg),clamp(shadow*mix(b,d,c),0.0,1.0));
  col=mix(col,col*mix(vec3_splat(luminance(horizonCol)),horizonCol,0.35),rain*0.45);
  col+=pow(alpha,4.0)*mix(horizonCol,zenithCol,0.65)*(1.0-rain)*0.12;
  return vec4(col,alpha);
}

#ifdef NL_AURORA
vec4 renderAurora(vec3 p,float t,float rain,vec3 FOG_COLOR) {
  t*=NL_AURORA_VELOCITY;
  p.xz*=NL_AURORA_SCALE;
  p.xz+=0.05*sin(p.x*4.0+20.0*t);
  float d0=sin(p.x*0.1+t+sin(p.z*0.2));
  float d1=sin(p.z*0.1-t+sin(p.x*0.2));
  float d2=sin(p.z*0.1+sin(d0+d1*2.0)+d1*2.0+d0);
  d0*=d0;d1*=d1;d2*=d2;
  d2=d0/(1.0+d2/NL_AURORA_WIDTH);
  float mask=(1.0-0.8*rain)*max(1.0-4.0*max(FOG_COLOR.b,FOG_COLOR.g),0.0);
  vec3 col=mix(NL_AURORA_COL1,NL_AURORA_COL2,d1);
  col=mix(col,mix(NL_AURORA_COL1,NL_AURORA_COL2,0.5),0.35*sin(d1*6.2831)*sin(d1*6.2831));
  return vec4(NL_AURORA*col,1.0)*d2*mask;
}
#endif

vec4 nlCloudAuroraReflection(nl_skycolor skycol,nl_environment env,vec3 viewDir,vec3 wPos,vec3 CAMERA_POS,highp float t) {
  vec2 cloudPos=wPos.xz;
  cloudPos+=(187.0-(wPos.y+CAMERA_POS.y))*viewDir.xz/viewDir.y;
  float fade=clamp(2.0-0.005*length(cloudPos),0.0,1.0);
  cloudPos+=CAMERA_POS.xz;
  vec4 refl=vec4_splat(0.0);
  #ifdef NL_AURORA
    vec4 aurora=renderAurora(cloudPos.xyy,t,env.rainFactor,env.fogCol);
    aurora.a*=fade;
    refl=vec4(2.0*aurora.rgb*aurora.a,aurora.a);
  #endif
  #if NL_CLOUD_TYPE == 1
    vec4 clouds=renderCloudsSimple(skycol,cloudPos.xyy,t,env.rainFactor);
    clouds.a*=fade;
    refl=vec4(mix(refl.rgb,clouds.rgb,clouds.a),min(refl.a+clouds.a,1.0));
  #endif
  return refl;
}

#endif
