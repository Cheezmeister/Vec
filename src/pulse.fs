
/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
    uniform float ticks;
    uniform float a = 1.0;
    uniform float hue = 0.5;

void main() {
    float phase = ticks * 1 / 1000.0;
    float h = hue;
    float s = 1.0;
    float v = 0.2 + 0.8 * nsin(phase * mPI);
    vec3 rgb = hsv2rgb(vec3(h, s, v));
    gl_FragColor = vec4(rgb, a);
}
)
