/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"
STRINGIFY(

    varying vec4 glPos;
    attribute vec4 inPos;
    uniform vec2 offset;
    uniform float rotation;
    uniform float ticks;
    uniform float scale = 1;
    const float frequency = 2;
    const float mPI = 3.14159;

void main() {
    vec2 rotated;
    rotated.x = inPos.x * cos(rotation) - inPos.y * sin(rotation);
    rotated.y = inPos.x * sin(rotation) + inPos.y * cos(rotation);
    float phase = ticks * frequency / 1000.0;
    vec2 pos = rotated * (0.2 + 0.02 * sin(phase * mPI));
    pos *= scale;
    gl_Position = glPos = vec4(offset + pos, 0, 1);
}

)
#undef STRINGIFY
