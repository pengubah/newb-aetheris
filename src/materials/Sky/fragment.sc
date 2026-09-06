#ifndef INSTANCING
  $input v_worldPos, v_underwaterRainTimeDay, v_fogColor
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>
  uniform vec4 TimeOfDay;
  uniform vec4 FogColor;
  uniform vec4 FogAndDistanceControl;
  uniform vec4 ViewPositionAndTime;
#endif

SAMPLER2D_AUTOREG(s_noisevoxels);

float pow2(float x){return x*x;}
float pow1_5(float x){return pow(x,1.5);}
float clamp01(float x){return clamp(x,0.0,1.0);}
float sqrt1(float x){return sqrt(max(x,0.0));}

float cubicFollowNoise(vec2 p){
    vec2 cell = vec2(1.0 / 128.0);
    vec2 quantized = floor(p / cell) * cell + cell * 2.0;
    return texture2D(s_noisevoxels, quantized).r;
}

vec3 GetAurora(vec3 viewDir, vec4 ViewPositionAndTime, float dither) {
    float VdotU = clamp(viewDir.y, 0.0, 1.0);
    float visibility = sqrt1(clamp01(VdotU * 4.5 - 0.25));
    visibility *= 8.0 - VdotU * 0.9;
    if (visibility <= 1.0) return vec3(0.0, 0.0, 0.0);

    vec3 aurora = vec3(0.0, 0.0, 0.0);
    vec3 wpos = viewDir;
    wpos.xz /= max(wpos.y, 0.1);

    vec2 cameraPosM = vec2(ViewPositionAndTime * 0.0, 0.0);

    const int sampleCount = 10;
    const int sampleCountP = sampleCount + 10;

    float ditherM = dither + 10.0;

    for (int i = 0; i < sampleCount; i++) {
        float current = pow2((float(i) + ditherM) / float(sampleCountP));
        float currentM = 1.0 - current;

        vec2 planePos = wpos.xz * (0.9 + current) * 10.0 + cameraPosM;
        planePos *= 0.003;

        float noise = cubicFollowNoise(planePos);
        noise = pow2(pow2(pow2(1.0 - 1.0 * abs(noise - 0.4))));

        float anim1 = cubicFollowNoise(planePos * 0.5 + ViewPositionAndTime * 0.00);
        float anim2 = cubicFollowNoise(planePos * 0.8 - ViewPositionAndTime * 0.00);
        noise *= mix(anim1, anim2, 0.5);

        aurora += noise * currentM * mix(vec3(0.65, 0.48, 1.35), vec3(0.0, 5.55, 1.85), pow2(pow2(currentM)));
    }

    aurora *= 0.8;
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

    vec3 skyColor = nlRenderSky(skycol, env, -viewDir, v_underwaterRainTimeDay.z, true);
    #ifdef NL_OVERWORLD_STARS
      skyColor += NL_OVERWORLD_STARS * nlRenderOverworldStars(-viewDir, env);
    #endif

  float dither = fract(sin(dot(uv, vec2(12.9898,78.233))) * 43758.5453);
    vec3 aurora = GetAurora(viewDir, vec4 ViewPositionAndTime, dither);

    skyColor = colorCorrection(skyColor);

    gl_FragColor = vec4(skyColor, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
