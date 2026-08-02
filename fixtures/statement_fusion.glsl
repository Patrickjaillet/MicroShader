void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv=fragCoord/iResolution.xy;
    float r=uv.x;
    float g=uv.y;
    float b=0.5;
    r=r*2.0;
    g=g*2.0;
    b=b*2.0;
    r=r+0.1;
    g=g+0.1;
    b=b+0.1;
    fragColor=vec4(r,g,b,1.0);
}
