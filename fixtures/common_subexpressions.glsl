float sdSphere(vec3 p, float r)
{
    float distanceToOrigin = length(p);
    float radiusScaled = r * 2.0;
    return distanceToOrigin - radiusScaled;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 p = vec3(uv, 0.0);
    float a = dot(p, p);
    float b = dot(p, p);
    float c = sin(iTime) * 0.5;
    float d = sin(iTime) * 0.5;
    fragColor = vec4(a, b, c, d);
}
