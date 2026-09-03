$input v_color0
#include <newb/config.h>
#if NL_CLOUD_TYPE == 0 || NL_CLOUD_TYPE >= 2
  $input v_color1, v_color2
#endif
#if NL_CLOUD_TYPE == 0 || NL_CLOUD_TYPE >= 2
  $input v_dayFactor
#endif

#include <bgfx_shader.sh>
#include <newb/main.sh>

uniform vec4 CameraPosition;

#define NL_CLOUD_PARAMS(x) NL_CLOUD2##x##THICKNESS, NL_CLOUD2##x##RAIN_THICKNESS, NL_CLOUD2##x##VELOCITY, NL_CLOUD2##x##SCALE, NL_CLOUD2##x##DENSITY, NL_CLOUD2##x##SHAPE

#if NL_CLOUD_TYPE == 0

// Cloud algorithm extracted from the shader-editor preview and adapted
// to the Minecraft/Aetheris fragment pipeline.
float vanillaCloudRand(highp vec2 n) {
  return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

vec4 vanillaClouds(vec2 uv, vec3 sky, float fade, float t) {
  float a = 0.0;
  float isTime = t * NL_CLOUD0_CLOUD_SPEED;
  vec4 col;

  uv *= NL_CLOUD0_SIZE;

  for (int i = 0; i < NL_CLOUD0_STEPS; i++) {
    uv /= 1.007;
    float c = step(0.7, vanillaCloudRand(floor(uv + isTime)));
    a = mix(a, 1.0, c);
  }

  vec2 b = vec2(step(0.7, vanillaCloudRand(floor(uv + isTime))));
  vec3 ccol = NL_CLOUD0_SIDECOL;
  ccol = mix(ccol, NL_CLOUD0_BOTTOMCOL, b.x);

  col.rgb = sky;
  col.rgb = mix(col.rgb, ccol, a * fade);
  col.a = a * fade * NL_CLOUD0_VISIBLE;

  return col;
}

#endif

void main() {
  vec4 color = v_color0;

  #if NL_CLOUD_TYPE == 0
    // Equivalent of the preview's gl_FragCoord.xy / resolution.xy.
    // bgfx provides the viewport rectangle as u_viewRect.xyzw.
    vec2 uv = (gl_FragCoord.xy - u_viewRect.xy) / u_viewRect.zw;

    // Keep the preview's p/gradient/camera-ray construction intact.
    vec3 p = vec3(uv, 1.0);
    vec2 cp = p.xz / p.y;

    // Use Aetheris sky colors instead of the preview's hard-coded blue.
    float gradient = smoothstep(0.001, 0.8, p.y);
    vec3 skyColor = mix(v_color2.rgb, v_color1.rgb, gradient);

    float fade = smoothstep(0.2, 0.9, p.y);

    color = vanillaClouds(cp, skyColor, fade, v_color2.a);

    // Keep the existing cloud-distance fade from the vertex stage.
    color.a *= v_color0.a;

    #ifdef NL_AURORA
      color.rgb += renderAurora(v_color0.xyz, v_color2.a, v_color1.a, v_dayFactor) * (1.0 - color.a);
    #endif

    color.rgb = colorCorrection(color.rgb);

  #elif NL_CLOUD_TYPE >= 2
    vec3 vDir = normalize(v_color0.xyz);
    vec3 cloudPos = v_color0.xyz;
    cloudPos.xz += CameraPosition.xz;

    #if NL_CLOUD_TYPE == 2
      color = renderCloudsRounded(vDir, cloudPos, v_color1.w, v_color2.w, v_color2.rgb, v_color1.rgb, NL_CLOUD_PARAMS(_));

      #ifdef NL_CLOUD2_LAYER2
        vec2 parallax = vDir.xz / abs(vDir.y) * NL_CLOUD2_LAYER2_OFFSET;
        vec3 offsetPos = cloudPos;
        offsetPos.xz += parallax;
        vec4 color2 = renderCloudsRounded(vDir, offsetPos, v_color1.a, v_color2.a*2.0, v_color2.rgb, v_color1.rgb, NL_CLOUD_PARAMS(_LAYER2_));
        color = mix(color2, color, 0.2 + 0.8*color.a);
      #endif

      #ifdef NL_AURORA
        color += renderAurora(cloudPos, v_color2.a, v_color1.a, vec3(v_dayFactor, v_dayFactor, v_dayFactor))*(1.0-0.95*color.a);
      #endif

      color.a *= v_color0.a;
    #else
      vDir.xz *= 0.3 + v_color0.w; // height parallax

      vec2 p = (vDir.xz)/(0.015 + 0.035*abs(vDir.y));
      p += 0.035*CameraPosition.xz;

      vec4 clouds = renderClouds(p, v_color2.w, v_color1.w, v_color2.rgb, v_color1.rgb, NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);
      color = clouds;

      #ifdef NL_AURORA
        p.xy *= 34.7;
        color += renderAurora(p.xyy, v_color2.w, v_color1.w, vec3(v_dayFactor, v_dayFactor, v_dayFactor))*(1.0-0.95*color.a);
      #endif

      color.a *= smoothstep(0.0, 0.7, vDir.y);
    #endif

    color.rgb = colorCorrection(color.rgb);
  #endif

  gl_FragColor = color;
}
