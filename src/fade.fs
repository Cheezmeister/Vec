/* This file is (ab)used by the C preprocessor
   to embed shaders in gfx.cpp at compile time. */
#include "common.glsl"

STRINGIFY(
varying vec4 glPos; 
uniform sampler2D texFramebuffer; 

void main() { 
  gl_FragColor = texture2D(texFramebuffer, glPos.xy) * 0.6; 
} 
)
