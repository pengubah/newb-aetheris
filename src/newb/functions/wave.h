#ifndef WAVE_H
#define WAVE_H

#include "utils.h"
#include "noise.h"

#ifdef NL_LANTERN_WAVE
void lanternWave(
  inout vec3 worldPos, vec3 cPos, vec3 bPos, vec2 bPosC, float texPosY, float rainFactor, vec2 uv1, float windStrength, highp float t
) {
  bool y6875=bPos.y==0.6875;
  bool y5625=bPos.y==0.5625;
  bool isLantern=((y6875||y5625)&&bPosC.x==0.125)||((y5625||bPos.y==0.125)&&bPosC.x==0.1875);
  bool isChain=bPosC.x==0.0625&&y6875;
  if(y5625&&(texPosY<0.001||(texPosY>0.43&&texPosY<0.93))) isLantern=false;

  if(uv1.x>0.58&&(isChain||isLantern)) {
    float phase=dot(floor(cPos),vec3_splat(0.3927));
    float gust=noise1D(t*0.45+phase)+0.35*noise1D(t*0.83-phase);
    vec2 theta=vec2(t+phase,t*1.27+phase*0.7);
    theta=sin(vec2(theta.x,theta.x+0.7))+rainFactor*0.65*sin(vec2(theta.y,theta.y+0.9));
    theta*=NL_LANTERN_WAVE*windStrength*(0.76+0.12*gust);
    vec2 sinA=sin(theta),cosA=cos(theta);
    vec3 pivotPos=vec3(0.5,1.0,0.5)-bPos;
    worldPos.x+=dot(pivotPos.xy,vec2(1.0-cosA.x,-sinA.x));
    worldPos.y+=dot(pivotPos,vec3(sinA.x*cosA.y,1.0-cosA.x*cosA.y,sinA.y));
    worldPos.z+=dot(pivotPos,vec3(sinA.x*sinA.y,-cosA.x*sinA.y,1.0-cosA.y));
  }
}
#endif

#ifdef NL_EXTRA_PLANTS_WAVE
void extraPlantsFlag(inout bool shouldWave, vec2 uv0, bool isTop) {
  int texN=64*int(uv0.y*32.0)+int(uv0.x*64.0);
  if((texN>=18&&texN<=20)||(texN>=177&&texN<=180)||(texN>=186&&texN<=187)||(texN>=444&&texN<=459)||texN==678||(texN>=796&&texN<=797)||(texN>=803&&texN<=804)||(texN>=832&&texN<=834)||(texN>=837&&texN<=838)) {
    shouldWave=true;
  } else if(texN==6||texN==8||texN==25||texN==85||texN==110||texN==145||texN==192||texN==223||texN==336||texN==387||texN==390||texN==396||(texN>445&&texN<453)||(texN>=530&&texN<=531)||texN==564||texN==642||texN==679||(texN>=715&&texN<=717)||texN==753||(texN>=761&&texN<=763)||texN==773||texN==774||texN==808||texN==823||texN==861||texN==914||texN==915||texN==945||texN==1011||(texN>=1079&&texN<=1082)||texN==1084||(texN>=1090&&texN<=1092)||texN==1187) {
    shouldWave=isTop;
  } else if(texN==23||texN==547||texN==610) {
    shouldWave=!isTop;
  }
}
#endif

void nlWave(
  inout vec3 worldPos, inout vec3 light, float rainFactor, vec2 uv1, vec2 lit,
  vec2 uv0, vec3 bPos, vec4 COLOR, vec3 cPos, vec3 tiledCpos, highp float t, sampler2D terrainTex,
  bool isColored, float camDist, bool isTreeLeaves
) {
  if(camDist>NL_WAVE_RANGE) return;

  float range=saturate(camDist/max(NL_WAVE_RANGE,0.001));
  float waveFade=smoothstep(0.45,1.0,range);
  float texPosY=fract(uv0.y*float(textureSize(terrainTex,0).y)/16.0);
  vec2 bPosC=abs(bPos.xz-0.5);
  vec2 bPosH=fract(bPos.xz*2.0);
  bool isTop=texPosY<0.5;
  bool isPlants=COLOR.g>0.001&&COLOR.r/COLOR.g<1.9;
  bool isVines=(bPosC.x==0.453125&&bPos.z==0.0)||(bPosC.y==0.453125&&bPos.x==0.0);
  bool isFarmPlant=(bPos.y==0.9375)&&(bPosC.x==0.25||bPosC.y==0.25);
  bool isRedStone=COLOR.r>0.25&&COLOR.r>3.0*COLOR.g&&COLOR.b==0.0;
  bool isLeafLitter=bPos.y==0.015625&&(bPosH.x+bPosH.y)==0.0;
  bool shouldWave=((isTreeLeaves||isPlants||isVines)&&isColored&&!isLeafLitter)||(isFarmPlant&&isTop);

  float gust=0.62+0.38*noise1D(t*0.24+dot(cPos,vec3(0.11,0.07,0.13)));
  float windStrength=lit.y*(gust+0.42*rainFactor)*(1.0-waveFade);
  windStrength*=0.82+0.18*uv1.x;

  if(isFarmPlant&&!isTop) light*=0.74;
  if(isColored&&!isTreeLeaves&&!isLeafLitter&&uv0.y>0.214&&uv0.y<0.502&&!isRedStone) {
    float depth=saturate((0.72-bPos.y)*1.15);
    float plantShade=isTop?1.08:0.94-0.34*depth;
    light*=mix(plantShade,1.0,waveFade);
  }

  #ifdef NL_PLANTS_WAVE
    #ifdef NL_EXTRA_PLANTS_WAVE
      extraPlantsFlag(shouldWave,uv0,isTop);
    #endif
    if(shouldWave) {
      float wave=NL_PLANTS_WAVE*windStrength;
      if(isTreeLeaves) wave*=0.52;
      else if(isVines) wave*=0.38+0.62*fract(0.01+tiledCpos.y*0.5);
      else if(isPlants&&isColored&&!isTop) wave*=bPos.y>0.0?bPos.y-1.0:0.0;

      float phaseDiff=dot(cPos,vec3_splat(PI_QUART))+fastRand(tiledCpos.xz+tiledCpos.y);
      float baseWave=sin(t*NL_WAVE_SPEED+phaseDiff);
      float gustWave=sin(t*NL_WAVE_SPEED*1.37+phaseDiff*1.13);
      float sway=mix(baseWave,gustWave,0.35+0.45*rainFactor);
      float secondary=sin(t*NL_WAVE_SPEED*0.63-phaseDiff*0.7);
      vec3 displacement=vec3(wave*sway,0.22*wave*wave,0.82*wave*secondary);
      worldPos+=displacement;
    }
  #endif

  #ifdef NL_LANTERN_WAVE
    lanternWave(worldPos,cPos,bPos,bPosC,texPosY,rainFactor,uv1,windStrength,t);
  #endif
}

#endif
