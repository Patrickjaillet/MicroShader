vec2 uv=FC.xy/r.xy;vec4 bg=texture2D(b,uv);gl_FragColor=bg+vec4(uv,sin(t),1.0);
