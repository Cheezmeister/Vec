/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
varying vec4 glPos;
attribute vec4 inPos;

void main() {
    gl_Position = glPos = inPos;
}

)
#undef STRINGIFY
