#ifndef WAVE_H
#define WAVE_H

#include "utils.h"
#include "noise.h"

#ifdef NL_LANTERN_WAVE
void lanternWave(inout vec3 worldPos,vec3 cPos,vec3 bPos,vec2 bPosC,float texPosY,float rainFactor,vec2 uv1,float windStrength,highp float t) {
  bool y6875=bPos.y==0.6875;
  bool y5625=bPos.y==0.5625;
  bool isLantern=((y6875||y5625)&&bPosC.x==0.125)||((y5625||bPos.y==0.125)&&bPosC.x==0.1875);
  bool isChain=bPosC.x==0.0625&&y6875;
  if(y5625&&(texPosY<0.001||(texPosY>0.43&&texPosY<0.93))) isLantern=false;
  if(uv1.x>0.6&&(isChain||isLantern)) {
    float phase=dot(floor(cPos),vec3_splat(0.3927));
    float wind=sin(t+phase)+0.35*sin(1.7*t+phase*1.3)+rainFactor*(0.45*sin(2.7*t+phase)+0.2);
    float sway=NL_LANTERN_WAVE*windStrength*wind;
    float sway2=NL_LANTERN_WAVE*windStrength*(0.5+0.5*rainFactor)*sin(0.73*t+phase);
    float sx=sin(sway),cx=cos(sway),sz=sin(sway2),cz=cos(sway2);
    vec3 pivot=vec3(0.5,1.0,0.5)-bPos;
    worldPos.x+=dot(pivot.xy,vec2(1.0-cx,-sx));
    worldPos.y+=dot(pivot,vec3(sx*cz,1.0-cx*cz,sz));
    worldPos.z+=dot(pivot,vec3(sx*sz,-cx*sz,1.0-cz));
  }
}
#endif

#ifdef NL_EXTRA_PLANTS_WAVE
void extraPlantsFlag(inout bool shouldWave,vec2 uv0,bool isTop) {
  int texN=64*int(uv0.y*32.0)+int(uv0.x*64.0);
  if((texN>=18&&texN<=20)||(texN>=177&&texN<=180)||(texN>=186&&texN<=187)||(texN>=444&&texN<=459)||texN==678||(texN>=796&&texN<=797)||(texN>=803&&texN<=804)||(texN>=832&&texN<=834)||(texN>=837&&texN<=838)) shouldWave=true;
  else if(texN==6||texN==8||texN==25||texN==85||texN==110||texN==145||texN==192||texN==223||texN==336||texN==387||texN==390||texN==396||(texN>445&&texN<453)||(texN>=530&&texN<=531)||texN==564||texN==642||texN==679||(texN>=715&&texN<=717)||texN==753||(texN>=761&&texN<=763)||texN==773||texN==774||texN==808||texN==823||texN==861||texN==914||texN==915||texN==945||texN==1011||(texN>=1079&&texN<=1082)||texN==1084||(texN>=1090&&texN<=1092)||texN==1187) shouldWave=isTop;
  else if(texN==23||texN==547||texN==610) shouldWave=!isTop;
}
#endif

void nlWave(inout vec3 worldPos,inout vec3 light,float rainFactor,vec2 uv1,vec2 lit,vec2 uv0,vec3 bPos,vec4 COLOR,vec3 cPos,vec3 tiledCpos,highp float t,sampler2D terrainTex,bool isColored,float camDist,bool isTreeLeaves) {
  if(camDist>NL_WAVE_RANGE) return;
  float waveFade=2.0*max(camDist/NL_WAVE_RANGE-0.5,0.0);
  waveFade*=waveFade;
  float texPosY=fract(uv0.y*vec2(textureSize(terrainTex,0)).y/16.0);
  vec2 bPosC=abs(bPos.xz-0.5);
  vec2 bPosH=fract(bPos.xz*2.0);
  bool isTop=texPosY<0.5;
  bool isPlants=COLOR.r/max(COLOR.g,0.001)<1.9;
  bool isVines=(bPosC.x==0.453125&&bPos.z==0.0)||(bPosC.y==0.453125&&bPos.x==0.0);
  bool isFarmPlant=(bPos.y==0.9375)&&(bPosC.x==0.25||bPosC.y==0.25);
  bool isRedStone=COLOR.r>0.25&&COLOR.r>3.0*COLOR.g&&COLOR.b==0.0;
  bool isLeafLitter=bPos.y==0.015625&&(bPosH.x+bPosH.y)==0.0;
  bool shouldWave=((isTreeLeaves||isPlants||isVines)&&isColored&&!isLeafLitter)||(isFarmPlant&&isTop);
  float windStrength=lit.y*(0.55+0.45*noise1D(t*0.36)+rainFactor*0.55)*(1.0-waveFade);
  light*=isFarmPlant&&!isTop?0.7:1.1;
  if(isColored&&!isTreeLeaves&&!isLeafLitter&&uv0.y>0.214&&uv0.y<0.502&&!isRedStone) light*=mix(isTop?1.2:1.2-1.2*(bPos.y>0.0?1.5-bPos.y:0.5),1.0,waveFade);

  #ifdef NL_PLANTS_WAVE
    #ifdef NL_EXTRA_PLANTS_WAVE
      extraPlantsFlag(shouldWave,uv0,isTop);
    #endif
    if(shouldWave) {
      float wave=NL_PLANTS_WAVE*windStrength;
      if(isTreeLeaves) wave*=0.48;
      else if(isVines) wave*=fract(0.01+tiledCpos.y*0.5);
      else if(isPlants&&isColored&&!isTop) wave*=bPos.y>0.0?bPos.y-1.0:0.0;
      float phaseDiff=dot(cPos,vec3_splat(PI_QUART))+fastRand(tiledCpos.xz+tiledCpos.y);
      float w0=sin(t*NL_WAVE_SPEED+phaseDiff);
      float w1=sin(t*NL_WAVE_SPEED*1.47+phaseDiff*1.13);
      float w2=sin(t*NL_WAVE_SPEED*0.61+phaseDiff*2.1);
      wave*=1.0+mix(w0,w1,rainFactor)+0.25*w2;
      worldPos.xyz-=vec3(wave,0.5*wave*wave,wave*0.92);
    }
  #endif

  #ifdef NL_LANTERN_WAVE
    lanternWave(worldPos,cPos,bPos,bPosC,texPosY,rainFactor,uv1,windStrength,t);
  #endif
}

#endif
