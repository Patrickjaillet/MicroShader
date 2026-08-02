precision mediump float;
uniform vec2 iResolution;
uniform float iTime;
uniform vec2 iMouse;
uniform sampler2D iChannel0;
void main(){vec2 uv=gl_FragCoord.xy/iResolution.xy;vec4 bg=texture2D(iChannel0,uv);gl_FragColor=bg+vec4(uv,sin(iTime),1.0);}
