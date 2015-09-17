/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
    varying vec4 glPos;
    uniform float phase;
    uniform float value = 1.0;
    uniform vec2 center;
    uniform float radius;
    uniform float thickness = 0.1;

void main() {
    float h = nsin(phase * mPI);
    float v = value;
    float len = length(vec2(glPos) - center);
    float s = thickness - min(thickness, abs(len - radius));
    vec3 rgb = hsv2rgb(vec3(h, s, v));
    gl_FragColor = vec4(rgb, 0);
}
)
