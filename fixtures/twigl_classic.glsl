precision mediump float;
uniform vec2 resolution;
uniform float time;
uniform vec2 mouse;
uniform sampler2D backbuffer;
void main(){vec2 uv=gl_FragCoord.xy/resolution.xy;vec4 bg=texture2D(backbuffer,uv);gl_FragColor=bg+vec4(uv,sin(time),1.0);}
