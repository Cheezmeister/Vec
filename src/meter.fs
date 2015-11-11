/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
    uniform float percent;
    uniform float ticks;
    varying vec4 glPos;
    varying vec4 glInPos;
    varying float filled;

void main() {
    float hue = nsin((ticks/3000 - glPos.y) * mPI);
    float filled = ((glInPos.y+1)/2.0 < percent ? 1 : 0);
    vec3 rgb = hsv2rgb(vec3(hue, 0.8, 1.0 * filled));
    float threshold = 0.9;
    gl_FragColor = vec4(rgb, 1);
}
)
