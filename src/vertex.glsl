#version 120
attribute vec4 inPos;
uniform vec2 offset;
varying vec4 glPos;
void main() {
    gl_Position = glPos = inPos + vec4(offset, 0, 1);
}
