/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
varying vec4 glPos;
/* uniform sampler2D texFramebuffer; */
uniform sampler2D texSource;
uniform vec2 dir;
uniform bool fromFB;
uniform float textureSize;

void main() {
  const float distance = 10;

  vec2 pos = (glPos.xy + vec2(1,1)) / 2.0;
  vec4 final = vec4(0);
  vec2 pixel = dir / textureSize;
  for (float i = -distance; i <= distance; ++i) {
    // FIXME Why the deuce do I have to add four here? Where is the offset coming from?
    vec2 voff = (i+4) * pixel;
    float weight = (1 - i / distance) / (distance * 2 + 1);
    final += texture2D(texSource, pos + voff) * weight;
  }

  gl_FragColor = final;
}
)
