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
uniform vec4 cameraPosition;

SAMPLER2D_AUTOREG(s_NoiseTex);

float pow2(float x) { return x * x; }
float pow1_5(float x) { return x * sqrt(x); }
float clamp01(float x) { return clamp(x, 0.0, 1.0); }
float sqrt1(float x) { return sqrt(max(0.0, x)); }

float sunVisibility = 0.0;
float rainFactor = 0.0;
float maxBlindnessDarkness = 0.0;
float moonPhase = 0.0;
float inSnowy = 1.0;

vec3 GetAuroraBorealis(vec3 viewPos, float VdotU, float dither) {
    float syncedTime = ViewPositionAndTime.w;
    float frameTimeCounter = ViewPositionAndTime.w;

    float visibility = sqrt1(clamp01(VdotU * 1.5 - 0.225)) - sunVisibility - rainFactor - maxBlindnessDarkness;
    visibility *= 1.0 - VdotU * 0.9;

    #if AURORA_CONDITION == 1 || AURORA_CONDITION == 3
        visibility -= moonPhase;
    #endif
    #if AURORA_CONDITION == 2 || AURORA_CONDITION == 3
        visibility *= inSnowy;
    #endif
    #if AURORA_CONDITION == 4
        visibility = max(visibility * inSnowy, visibility - moonPhase);
    #endif

    if (visibility > 0.0) {
        vec3 aurora = vec3(0.0,0.0,0.0);

        vec3 wpos = viewPos;
        wpos.xz /= max(0.0001, wpos.y);
        vec2 cameraPositionM = cameraPosition.xz * 0.0075;
        cameraPositionM.x += syncedTime * 0.04;

        int sampleCount = 20;
        int sampleCountP = sampleCount + 10;

        float ditherM = dither + 5.0;
        float auroraAnimate = frameTimeCounter * 0.001;

        for (int i = 0; i < sampleCount; i++) {
            float current = pow2((float(i) + ditherM) / float(sampleCountP));

            vec2 planePos = wpos.xz * (0.8 + current) * 12.0 + cameraPositionM;
            #if AURORA_STYLE == 1
                planePos = floor(planePos) * 0.0007;

                float n = texture2D(s_NoiseTex, planePos).b;
                n = pow2(pow2(pow2(pow2(1.0 - 2.0 * abs(n - 0.5)))));

                n *= pow1_5(texture2D(s_NoiseTex, planePos * 100.0 + auroraAnimate).b);
            #else
                planePos *= 0.00007;

                float n = texture2D(s_NoiseTex, planePos).r;
                n = pow2(pow2(pow2(pow2(1.0 - 2.0 * abs(n - 0.5)))));

                n *= texture2D(s_NoiseTex, planePos * 3.0 + auroraAnimate).b;
                n *= texture2D(s_NoiseTex, planePos * 5.0 - auroraAnimate).b;
            #endif

            float currentM = 1.0 - current;
            aurora += n * currentM * mix(vec3(7.0, 3.5, 17.0), vec3(5.0, 15.0, 17.0), pow2(pow2(currentM)));
        }

        #if AURORA_STYLE == 1
            aurora *= 1.3;
        #else
            aurora *= 1.8;
        #endif

        aurora *= 1.5;
        return aurora * visibility / float(sampleCount);
    }

    return vec3(0.0,0.0,0.0);
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

    float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    float VdotU = clamp(viewDir.y, 0.0, 1.0);
    vec3 auroraColor = GetAuroraBorealis(viewDir, VdotU, dither);
    skyColor += auroraColor; 

    skyColor = colorCorrection(skyColor);

    gl_FragColor = vec4(skyColor, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
