$input a_color0, a_position
#ifdef INSTANCING
  $input i_data0, i_data1, i_data2, i_data3
#endif
$output v_color0
#include <newb/config.h>
#if NL_CLOUD_TYPE >= 2
  $output v_color1, v_color2, v_dayFactor
#endif

#include <bgfx_shader.sh>
#include <newb/main.sh>

// uniform vec4 CloudColor;
uniform vec4 FogAndDistanceControl;
uniform vec4 ViewPositionAndTime;
uniform vec4 TimeOfDay;
uniform vec4 CameraPosition;

float fog_fade(vec3 wPos) {
  float cloudDist = length(wPos.xz);
  return 1.0-smoothstep(FogAndDistanceControl.z*0.85, FogAndDistanceControl.z, cloudDist);
}
void main() {
  #ifdef INSTANCING
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
  #else
    mat4 model = u_model[0];
  #endif

  float t = ViewPositionAndTime.w;
  float rain = detectRain(FogAndDistanceControl.xyz);

  nl_environment env;
  env.end = false;
  env.nether = false;
  env.underwater = false;
  env.rainFactor = rain;
  env = calculateSunParams(env, TimeOfDay.x, 0.0);

  nl_skycolor skycol = nlOverworldSkyColors(env);
  vec3 pos = a_position;
  vec3 worldPos;

  #if NL_CLOUD_TYPE <= 2

    vec4 color;

    #if NL_CLOUD_TYPE == 0
      pos.y *= (NL_CLOUD0_THICKNESS + rain*(NL_CLOUD0_RAIN_THICKNESS - NL_CLOUD0_THICKNESS));
      worldPos = mul(model, vec4(pos, 1.0)).xyz;
      worldPos.y += 2.0;

      // Type 0 clouds adapt to the actual sky at each cloud vertex.
      // Keep the base cloud brightness neutral so dawn/sunset colors are not
      // painted uniformly across the whole cloud layer. The local sky sample
      // supplies the time-of-day color and the viewing direction supplies the
      // spatial gradient, so only the part of the cloud facing the dawn/sun
      // picks up the warm color.
      vec3 baseZenith = mix(NL_DAY_ZENITH_COL, NL_NIGHT_ZENITH_COL * (0.68 + 0.30*(1.0-max(-env.dayFactor, 0.0))), step(env.dayFactor, 0.0));
      vec3 baseEdge = mix(NL_DAY_EDGE_COL, NL_NIGHT_EDGE_COL * (0.68 + 0.30*(1.0-max(-env.dayFactor, 0.0))), step(env.dayFactor, 0.0));
      float baseCloudLum = dot(baseZenith + baseEdge, vec3(0.2126, 0.7152, 0.0722));
      vec3 baseCloud = vec3_splat(baseCloudLum);

      vec3 cloudViewDir = normalize(worldPos - CameraPosition.xyz);
      vec3 localSky = renderOverworldSky(skycol, env, cloudViewDir, true);
      float localLum = max(dot(localSky, vec3(0.2126, 0.7152, 0.0722)), 0.001);
      vec3 localTint = localSky / localLum;

      // Dawn/sunset color is localized to the sun-facing part of the cloud
      // layer. Keep a very small residual influence on the night side so the
      // transition still feels atmospheric, but prevent the whole night sky
      // from being washed by the dawn horizon.
      float dawnMask = nlDawnFactor(env.dayFactor);
      dawnMask *= nlNightDawnAttenuation(env.dayFactor);
      float sunFacing = smoothstep(
        0.0, 0.55, max(dot(env.sunDir, cloudViewDir), 0.0)
      );
      float dawnHorizon = 1.0 - smoothstep(0.05, 0.65, abs(cloudViewDir.y));
      float dawnInfluence = dawnMask * (0.10 + 0.90*sunFacing);
      dawnInfluence *= 0.35 + 0.65*dawnHorizon;

      // Daytime clouds remain mostly neutral; warm dawn/sunset tint becomes
      // strongest only where the low sun is actually visible.
      float tintStrength =
        0.10 + 0.30*dawnInfluence + 0.06*step(env.dayFactor, 0.0);
      color.rgb = baseCloud * mix(vec3_splat(1.0), localTint, tintStrength);

      // Let the local sky brightness darken clouds at night and softly brighten
      // them near the illuminated part of the horizon during dawn/day.
      float localLight = clamp(localLum * 2.2, 0.22, 1.25);
      color.rgb *= localLight;
      color.rgb += dot(color.rgb, vec3(0.3,0.4,0.3))*a_position.y;
      color.rgb *= 1.0 - 0.8*rain;
      color.rgb = colorCorrection(color.rgb);
      color.a = NL_CLOUD0_OPACITY * fog_fade(worldPos.xyz);

      // clouds.png has two non-overlaping layers:
      // r=unused, g=layers, b=reference, a=unused
      // g=0 (layer 0), g=1 (layer 1)
      bool isL2 = a_color0.g > 0.5 * a_color0.b;
      if (isL2) {
        #ifdef NL_CLOUD0_MULTILAYER
          worldPos.y += 64.0;
        #else
          worldPos = vec3(0.0,0.0,0.0);
          color.a = 0.0;
        #endif
      }
    #else
      pos.y *= 0.01;
      worldPos.xyz = mul(model, vec4(pos, 1.0)).xyz;

      float fade = fog_fade(worldPos.xyz);
      #if NL_CLOUD_TYPE == 1
        // make cloud plane spherical
        float len = length(worldPos.xz)*0.01;
        worldPos.y -= len*len*clamp(0.2*worldPos.y, -1.0, 1.0);

        vec3 cloudPos = worldPos;
        cloudPos.xz += CameraPosition.xz;

        color = renderCloudsSimple(skycol, cloudPos, t, rain);

        // cloud depth
        worldPos.y -= NL_CLOUD1_DEPTH*color.a*3.3;

        color.a *= NL_CLOUD1_OPACITY;

        #ifdef NL_AURORA
          color += renderAurora(cloudPos, t, rain, env.fogCol)*(1.0-color.a);
        #endif

        color.a *= fade;
        color.rgb = colorCorrection(color.rgb);
      #else // NL_CLOUD_TYPE 2
        v_dayFactor = env.dayFactor;
        v_color1 = vec4(skycol.zenith, rain);
        v_color2 = vec4(skycol.horizonEdge, ViewPositionAndTime.w);
        color = vec4(worldPos, fade);
      #endif 
    #endif

    v_color0 = color;
    gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));
  #else
    vec4 apos = vec4(pos.xz - 32.0, 1.0, 1.0);
    apos.x *= pos.y - 0.5;
    apos.xy = clamp(apos.xy, -1.0, 1.0);

    #if BGFX_SHADER_LANGUAGE_GLSL
      float h = model[3][1];
    #else
      float h = model[1][3];
    #endif
    h = clamp(0.002*h, 0.0, 1.0);

    worldPos = mul(u_invViewProj, apos).xyz;

    v_dayFactor = env.dayFactor;
    v_color0 = vec4(worldPos, h*h);
    v_color1 = vec4(skycol.zenith, rain);
    v_color2 = vec4(skycol.horizonEdge, ViewPositionAndTime.w);
    gl_Position = apos;
  #endif
}
