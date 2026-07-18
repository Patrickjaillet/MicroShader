void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    float brightness = uv.x * 1.0;
    float shifted = uv.y + 0.0;
    float unshifted = brightness - 0.0;
    float scaled = unshifted / 1.0;
    float leftIdentity = 1.0 * scaled;
    float leftZero = 0.0 + leftIdentity;
    float squared = pow(shifted, 2.0);
    fragColor = vec4(squared, leftZero, scaled, 1.0);
}
