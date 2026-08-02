void mainImage(out vec4 fragColor,in vec2 fragCoord){
float floorField=floor(fragCoord.x)+floor(fragCoord.y);
float fractField=fract(fragCoord.x)+fract(fragCoord.y);
float finalField=floorField+fractField+floor(floorField)+fract(fractField);
float filterField=finalField+floorField+fractField;
fragColor=vec4(floorField+fractField+finalField+filterField);
}