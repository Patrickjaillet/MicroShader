float mapA(vec3 p)
{
    return dot(p,p)-1.0;
}

float mapB(vec3 p)
{
    return dot(p,p)-2.0;
}

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec3 p=vec3(fragCoord,0.0);
    float d=dot(p,p);
    fragColor=vec4(d,mapA(p),mapB(p),1.0);
}
