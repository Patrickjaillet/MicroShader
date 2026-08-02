void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    vec3 ro = vec3(0.0, 0.0, -3.0);
    vec3 rd = normalize(vec3(uv, 1.0));
    float t = 0.0;
    vec3 glow = vec3(0.0);
    while (true)
    {
        vec3 p = ro + rd * t;
        float d = length(p) - 1.0;
        glow += vec3(0.02 / max(d, 0.001));
        t += d;
        if (d < 0.001 || t > 20.0)
        {
            break;
        }
    }

    float x = 0.0;
    do
    {
        x += 0.1;
    } while (x < 1.0);

    vec3 col = glow * 0.1 + vec3(x, x * 0.5, x * 0.25) + vec3(t * 0.02, uv.x, uv.y);
    fragColor = vec4(col, 1.0);
}
