/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
    attribute vec4 inPos;
    uniform float scale = 1;
    uniform float percent;
    varying vec4 glPos;
    varying vec4 glInPos;

void main() {
    float x = (inPos.x-1) * 0.05 + 1;
    float y = inPos.y * 0.5;
    gl_Position = glPos = vec4(x, y, 0, 1);
    glInPos = inPos;
}
)
