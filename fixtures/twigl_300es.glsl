#version 300 es
out vec4 outColor;
precision mediump float;
uniform vec2 resolution;
uniform float time;
uniform vec2 mouse;
uniform sampler2D backbuffer;
void main(){vec2 uv=gl_FragCoord.xy/resolution.xy;vec4 bg=texture2D(backbuffer,uv);outColor=bg+vec4(uv,sin(time),1.0);}
