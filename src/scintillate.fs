/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
    varying vec4 glPos;
    uniform float phase;
    uniform float value = 1.0;

void main() {
    float h = nsin(phase * mPI);
    float s = 1.0;
    float v = value;
    vec3 rgb = hsv2rgb(vec3(h, s, v));
    gl_FragColor = vec4(rgb, 0);
}
)
#undef STRINGIFY
