#ifndef NL_PLAYER_SHADOW_H
#define NL_PLAYER_SHADOW_H

#ifdef NL_PLAYER_SHADOW

float nlInRect(vec2 pos, float x1, float y1, float x2, float y2, float focus) {
  return min(
    1.0,
    max(min(min(pos.x - x1, x2 - pos.x), min(pos.y - y1, y2 - pos.y)), 0.0) / focus
  );
}

float nlPlayerShadow(vec3 wPos, float time, float horizontalMotion) {
  vec3 lookvector = wPos;
  lookvector.x *= 2.0;

  vec3 pos = lookvector + vec3(0.4, 0.4, 0.4);
  vec3 dir = vec3(-1.0, 1.25 * 0.31, 0.0);
  float factor = 1.0;

  if (pos.x < 0.2) {
    factor = max(0.0, pos.x / 0.4 + 0.5);
  }

  pos += dir * pos.x;

  float walkAmount = smoothstep(
    NL_PLAYER_SHADOW_MOTION_START,
    NL_PLAYER_SHADOW_MOTION_END,
    horizontalMotion
  );
  float footwalk = 0.0;
  float handswalk = 0.0;

  #ifdef NL_PLAYER_SHADOW_ANIMATION
    float walkPhase = time * NL_PLAYER_SHADOW_ANIMATION_SPEED;
    footwalk = sin(walkPhase) * NL_PLAYER_SHADOW_LEG_SWING * walkAmount;
    handswalk = sin(walkPhase + 3.14159265) *
                NL_PLAYER_SHADOW_HAND_SWING * walkAmount;
  #endif

  pos.yz -= vec2(0.2, 0.4);

  float body = max(
    nlInRect(pos.yz, -1.5 + footwalk * 0.4, -0.25, 0.75, 0.1, NL_PLAYER_SHADOW_FOCUS),
    nlInRect(pos.yz, -1.5 - footwalk * 0.4, -0.1, 0.75, 0.25, NL_PLAYER_SHADOW_FOCUS)
  );
  float hands = max(
    nlInRect(pos.yz, -0.5 + handswalk, -0.5, 0.25, 0.1, NL_PLAYER_SHADOW_FOCUS),
    nlInRect(pos.yz, -0.5 - handswalk, -0.1, 0.25, 0.5, NL_PLAYER_SHADOW_FOCUS)
  );

  return min(1.0, max(body, hands)) * factor;
}

#endif

#endif
