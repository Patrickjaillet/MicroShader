void mainImage(out vec4 fragColor,in vec2 fragCoord){
    float unused=1.0;
    float a=2.0*1.0;
    float b=a;
    if(b>0.0){b=b-1.0;}else{b=b+1.0;}
    fragColor=vec4(b);
}
