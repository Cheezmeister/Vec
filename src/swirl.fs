/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
varying vec4 glPos; 
uniform float ticks;

void main() { 
  float modticks = ticks;
  while (modticks > 1000.0) modticks -= 1000.0;
  /* float h = length(glPos) * modticks / 1000.0; */
  float h = nsin(ticks / 1000.0) * length(glPos);
  /* vec4 apos = glPos - vec4(0.1,0.1, 0, 0); */
  /* h += length(apos) * mPI; */
  float s = 0.5;
  float v = 0.1;
  vec3 rgb = hsv2rgb(vec3(h, s, v));
  vec3 rgb2 = hsv2rgb(vec3(h, s, v));
  gl_FragColor = vec4((rgb + rgb2)/2, 0);
} 
)
