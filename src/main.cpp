#include <stdio.h>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

GLFWwindow* window;

const char * vertexShaderSource = R""""(
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
        verts[gl_VertexID].position[0],
        verts[gl_VertexID].position[1]
    );
}

void main(){
    gl_Position = vec4(unpackPos(), 1.0f);
    uv = unpackUv();
}
)"""";


const char * fragShaderSource = R""""(
#version 460 core
in vec2 uv;

out vec4 FragColor;

void main(){
    // FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    FragColor = vec4((uv.xy+1.0f)/2.0f, 0.0f, 1.0f);
}
)"""";          

unsigned int compileShader(const char* shaderSource, GLenum shaderType){
    // Get a shader handle from OpenGL
    unsigned int shader = glCreateShader(shaderType);
    // provide the GLSL source and compile it
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    // Check for errors
    struct {
        int success;
        char infoLog[512];
    } shaderStat;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderStat.success);
    // Print errors if they occurred
    if(!shaderStat.success){
        glGetShaderInfoLog(shader, 512, NULL, shaderStat.infoLog);
        printf("compiling ");
        switch(shaderType){
            case GL_VERTEX_SHADER:   printf("vertex ");   break;
            case GL_FRAGMENT_SHADER: printf("fragment "); break;
        }
        printf("shader failed: %d\n%s\n", shaderStat.success, shaderStat.infoLog);

        glDeleteShader(shader);
        return 0;
    }

    // Give the shader handle back to caller
    return shader;
}

void keyHandler(GLFWwindow* window, int key, int scancode, int action, int modes){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE){
        glfwSetWindowShouldClose(window, true);
    }
}

void glfwErrorPrinter(int code, const char* desc){
    printf("Error code: %d\n%s\n", code, desc);
}

void glErrorPrinter(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void * userParam){
    printf("GL error\n\tsource: %d\n\ttype: %d\n\tid: %d\n\tseverity: %d\n\t%s", source, type, id, severity, message);
}

bool init(){
    glfwSetErrorCallback(glfwErrorPrinter);

    if(!glfwInit())
    {
        printf("Failed to initialize GLFW");
        return false;
    }

    window = glfwCreateWindow(400, 400, "Hello, world!", NULL, NULL);

    glfwSetKeyCallback(window, keyHandler);

    glfwMakeContextCurrent(window);

    // Load an OpenGL extension loader
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        printf("Failed to get GL Loader\n");
        return false;

    }

    glDebugMessageCallback(glErrorPrinter, nullptr);

    // Enable V-Sync
    glfwSwapInterval(1);

    // Display the window
    glfwShowWindow(window);

    // Set the bg color
    glClearColor(.1, .1, .1, 0.0);


    return true;
}

void loop(){
    // Triangle vertex data
    struct {
        float pos[3];
        float uv[2];
    } triangleVerts[] = {
        { {-1, -1, 0}, {-1, -1} },
        { { 1, -1, 0}, { 1, -1} },
        { { 0,  1, 0}, { 0,  1} },
    };

    GLuint ssbo;
    glCreateBuffers(1, &ssbo);
    glNamedBufferStorage(ssbo, sizeof(triangleVerts), triangleVerts, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    unsigned int shaderProg = glCreateProgram();
    {
        auto vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
        auto fragShader   = compileShader(fragShaderSource,   GL_FRAGMENT_SHADER);
        glAttachShader(shaderProg, vertexShader);
        glAttachShader(shaderProg, fragShader);
        glLinkProgram(shaderProg);
        GLint result;
        glGetProgramiv(shaderProg, GL_LINK_STATUS, &result);
        if(result == GL_FALSE)
        {
            char infoLog[512] = {0};
            GLsizei logLeng;
            glGetProgramInfoLog(shaderProg, 512, &logLeng, infoLog);
            printf("Error linking shader:\n%s\n", infoLog);
        }

        // Delete our shaders, gpu has them now.
        glDeleteShader(vertexShader);
        glDeleteShader(fragShader);

        if(result == GL_FALSE) return;
    }

    while(!glfwWindowShouldClose(window)){
        // Wiggle the top of the triangle and refresh the GPU's data
        // triangleVerts[6] = sin(glfwGetTime()/1);
        // glBufferSubData(GL_SHADER_STORAGE_BUFFER, 6*sizeof(float), sizeof(float), &triangleVerts[6]);

        // Actually clear the render buffer
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Set the shader to use
        glUseProgram(shaderProg);
        // Draw the triangle
        glDrawArrays(GL_TRIANGLES, 0 ,3);

        // Swap render and display buffers
        glfwSwapBuffers(window);

        glfwPollEvents();
    }
    

    // Unbind the storage buffer from ssbo
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

int main()
{
    // Set up window
    if(init())
    // Set up buffers and loop until esc pressed
    loop();

    // ---- Cleanup ----
    glfwDestroyWindow(window);
    glfwTerminate();
    glfwSetErrorCallback(NULL);
}