/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
    attribute vec4 inPos;
    uniform vec2 offset;
    uniform float rotation;
    uniform float ticks;
    uniform float scale = 1;
    varying vec4 glPos;
    const float frequency = 2;

void main() {
    vec2 rotated;
    rotated.x = inPos.x * cos(rotation) - inPos.y * sin(rotation);
    rotated.y = inPos.x * sin(rotation) + inPos.y * cos(rotation);
    float phase = ticks * frequency / 1000.0;
    vec2 pos = rotated * (0.2 + 0.02 * sin(phase * mPI));
    pos *= scale;
    pos.x += 0.02 * (inPos.x - inPos.y) * cos(phase * mPI);
    pos.y += 0.02 * (inPos.x - inPos.y) * sin(phase * mPI);
    gl_Position = glPos = vec4(offset + pos, 0, 1);
}
)
