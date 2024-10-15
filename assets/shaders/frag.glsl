#version 460 core
in vec2 uv;

uniform layout(binding=0) sampler2D tex1;
uniform layout(binding=1) sampler2D tex2;

uniform float time;

out vec4 FragColor;

const float pi = 3.1415926535;

void main(){
    FragColor = (texture(tex1, uv)*(sin(time)+1)/2.0f) + (texture(tex2, uv)*(sin(time+pi)+1)/2.0f);
}