vec3 greyFromScalar(float n) {
    vec3 grey = vec3(n, n, n);
    return grey;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 p = vec3(uv, 0.0);
    vec2 flat2 = vec2(uv.x, uv.x);
    vec3 chain = vec3(p.x, p.x, p.x);
    vec4 notAllEqual = vec4(p.x, p.x, p.x, 1.0);
    vec3 literalOnly = vec3(1.0, 1.0, 1.0);
    fragColor = vec4(greyFromScalar(uv.x) + chain + literalOnly, notAllEqual.w + flat2.x);
}
