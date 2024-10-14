#version 460 core
struct Vert {
    float position[3];
    float uv[2];
};

layout (binding = 0, std430) buffer ssbo {
    Vert[] verts;
};

out vec2 uv;

vec3 unpackPos(){
    return vec3(
        verts[gl_VertexID].position[0],
        verts[gl_VertexID].position[1],
        verts[gl_VertexID].position[2]
    );
}

vec2 unpackUv(){
    return vec2(
        verts[gl_VertexID].uv[0],
        verts[gl_VertexID].uv[1]
    );
}

void main(){
    gl_Position = vec4(unpackPos(), 1.0f);
    uv = unpackUv();
}