#version 460 core
in vec2 uv;

uniform layout(binding=0) sampler2D tex1;
uniform layout(binding=1) sampler2D tex2;

uniform float time;

out vec4 FragColor;

void main(){
    FragColor = texture(tex1, uv)*max(0,sin(time)) + texture(tex2, uv)*max(0,sin(time)*-1);
}