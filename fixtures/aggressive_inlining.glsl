float acc;

void tally(float v)
{
    acc=acc+v;
    acc=acc*0.5;
    acc=acc+1.0;
}

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    acc=0.0;
    tally(fragCoord.x);
    tally(fragCoord.y);
    fragColor=vec4(acc,acc,acc,1.0);
}
