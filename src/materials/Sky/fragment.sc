#ifndef INSTANCING
  $input v_worldPos, v_underwaterRainTimeDay, v_fogColor
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>
  uniform vec4 TimeOfDay;
  uniform vec4 FogColor;
  uniform vec4 FogAndDistanceControl;
#endif

uniform vec4 ViewPositionAndTime;

SAMPLER2D_AUTOREG(s_noisevoxels);

float pow2(float x){return x*x;}
float pow1_5(float x){return pow(x,1.5);}
float clamp01(float x){return clamp(x,0.0,1.0);}
float sqrt1(float x){return sqrt(max(x,0.0));}

float cubicFollowNoise(vec2 p){
    vec2 cell = vec2(1.0 / 1000.0);
    vec2 quantized = floor(p / cell) * cell + cell * cell * 3.0;
    return texture2D(s_noisevoxels, quantized).r;
}

vec3 GetAurora(vec3 viewDir, vec4 ViewPositionAndTime, float dither) {
    float VdotU = clamp(viewDir.y, 0.0, 1.0);
    float visibility = sqrt1(clamp01(VdotU * 4.5 - 0.35));
    visibility *= 8.0 - VdotU * 0.9;
    if (visibility <= 1.0) return vec3(0.0,0.0,0.0);

    vec3 aurora = vec3(0.0,0.0,0.0);
    vec3 wpos = viewDir;
    wpos.xz /= max(wpos.y, 0.1);

    vec2 cameraPosM = vec2(ViewPositionAndTime.w * 0.0, 0.0);

    const int sampleCount = 20;
    const int sampleCountP = sampleCount + 10;

    float ditherM = dither + 10.0;

    for (int i = 0; i < sampleCount; i++) {
        float current = pow2((float(i) + ditherM) / float(sampleCountP));
        float currentM = 1.0 - current;

        vec2 planePos = wpos.xz * (0.9 + current) * 2.5 + cameraPosM;
        planePos *= 0.02;

        float noise = cubicFollowNoise(planePos);
        noise = pow2(pow2(1.0 - 1.0 * abs(noise - 0.15)));

        float anim1 = cubicFollowNoise(planePos * 0.5 + ViewPositionAndTime.w * 0.0055);
        float anim2 = cubicFollowNoise(planePos * 0.5 - ViewPositionAndTime.w * 0.0055);
        noise *= mix(anim1, anim2, 1.0);

        aurora += noise * currentM *
            mix(vec3(0.65, 0.48, 1.8),
                vec3(0.0, 2.8, 2.0),
                pow2(pow2(currentM)));
    }

    aurora *= 0.4;
    return aurora * visibility / float(sampleCount);
}

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_worldPos);

    nl_environment env;
    env.end = false;
    env.nether = false;
    env.underwater = v_underwaterRainTimeDay.x > 0.5;
    env.rainFactor = v_underwaterRainTimeDay.y;
    env.dayFactor = v_underwaterRainTimeDay.w;
    env.fogCol = FogColor.rgb;
    env = calculateSunParams(env, TimeOfDay.x);

    nl_skycolor skycol = nlOverworldSkyColors(env);

    float mask = (1.0-1.0*env.rainFactor)*max(1.0 - 3.0*max(v_fogColor.b, v_fogColor.g), 0.0);

    vec3 skyColor = nlRenderSky(skycol, env, -viewDir, v_underwaterRainTimeDay.z, true);

    float dither = fract(sin(dot(viewDir.xy, vec2(12.9898,78.233))) * 43758.5453);
    vec3 aurora = GetAurora(viewDir, ViewPositionAndTime, dither) * mask;
    skyColor += aurora; 

    skyColor = colorCorrection(skyColor);

    gl_FragColor = vec4(skyColor, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
