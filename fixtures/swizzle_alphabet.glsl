void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 col = vec3(uv.x, uv.y, uv.x + uv.y);
    vec3 shifted = col.xyz + vec3(sin(iTime), cos(iTime), 0.0);
    vec2 swirl = shifted.xy - shifted.yx * 0.5;
    fragColor = vec4(swirl.xy, shifted.z, 1.0);
}
