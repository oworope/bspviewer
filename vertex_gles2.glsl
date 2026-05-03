#version 100
precision mediump float; // add precision qualifier

attribute vec3 aPos;
attribute vec3 aColor;

uniform mat4 uMVP;

varying vec3 vColor;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}