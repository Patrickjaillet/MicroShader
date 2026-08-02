void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    vec3 ro = vec3(0.0, 0.0, -3.0);
    vec3 rd = normalize(vec3(uv, 1.0));
    float t = 0.0;
    vec3 glow = vec3(0.0);
    float steps = 0.0;
    for (int rmi = 0; rmi < 80; rmi++)
    {
        steps += 1.0;
        vec3 p = ro + rd * t;
        float d = length(p) - 1.0;
        glow += vec3(0.01 / max(d, 0.001));
        if (d < 0.001)
        {
            break;
        }
        t += d;
    }

    vec2 z = uv * 2.0;
    vec2 c = vec2(-0.7, 0.27015);
    vec3 spark = vec3(0.0);
    float iter = 0.0;
    for (int fi = 0; fi < 64; fi++)
    {
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        spark += vec3(0.01, 0.02, 0.03);
        if (dot(z, z) > 4.0)
        {
            break;
        }
        iter += 1.0;
    }

    vec3 col = glow * 0.1 + spark + vec3(t * 0.05, z.x * 0.1, z.y * 0.1);
    fragColor = vec4(col, 1.0);
}
