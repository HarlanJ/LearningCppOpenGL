#include <stdio.h>
#include <cmath>
#include <fstream>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

GLFWwindow* window;


unsigned int loadCompileShader(const char* fName, GLenum shaderType)
{
    constexpr GLint bufSize = 256;

    std::ifstream fs;
    fs.open(fName, std::ifstream::in);

    if(fs.fail()){
        printf("Failed to open file \'%s\'\n", fName);
    }

    fs.seekg(0, fs.end);
    GLint len = fs.tellg();
    fs.seekg(0, fs.beg);

    printf("len: %d\n", len);

    char* src = new char[len];
    fs.read(src, len);
    printf("%.*s\n", len, src);

    // Get a shader handle from OpenGL
    unsigned int shader = glCreateShader(shaderType);
    // provide the GLSL source and compile it
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    delete[] src;

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
        { { -.95, -.95, 0}, { 0,  1} },
        { {  .95, -.95, 0}, { 1,  1} },
        { {  .95,  .95, 0}, { 1,  0} },

        { { -.95, -.95, 0}, { 0,  1} },
        { {  .95,  .95, 0}, { 1,  0} },
        { { -.95,  .95, 0}, { 0,  0} },
    };

    const struct{
        const char* img;
        const GLenum tex;
    } images[] = {
        {"assets/container.jpg", 0},
        {"assets/bird.jpg", 1},
    };

    for(size_t i = 0; i < 2; i++){
        unsigned int tex;

        struct {
            int width, height;
            int numCh;
        } imageStats;
        unsigned char* imageRaw = stbi_load(images[i].img, &imageStats.width, &imageStats.height, &imageStats.numCh, 0);
        
        if(imageRaw){
            glCreateTextures(GL_TEXTURE_2D, 1, &tex);
            glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureStorage2D(tex, 1, GL_RGBA32F, imageStats.width, imageStats.height);
            glTextureSubImage2D(tex, 0, 0, 0, imageStats.width, imageStats.height, GL_RGB, GL_UNSIGNED_BYTE, imageRaw);
            glBindTextureUnit(images[i].tex, tex);
        } else {
            printf("Failed to load texture\n");
        }

        stbi_image_free(imageRaw);
    }

    GLuint ssbo;
    glCreateBuffers(1, &ssbo);
    glNamedBufferStorage(ssbo, sizeof(triangleVerts), triangleVerts, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    unsigned int shaderProg = glCreateProgram();
    {
        auto vertexShader = loadCompileShader("assets/shaders/vertex.glsl", GL_VERTEX_SHADER);
        auto fragShader   = loadCompileShader("assets/shaders/frag.glsl",   GL_FRAGMENT_SHADER);
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

    const auto timeLoc = glGetUniformLocation(shaderProg, "time");

    while(!glfwWindowShouldClose(window)){
        // Clear the render buffer
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Set the shader to use
        glUseProgram(shaderProg);
        glUniform1f(timeLoc, glfwGetTime());
        // Draw the triangle
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap render and display buffers
        glfwSwapBuffers(window);

        glfwPollEvents();
    }
    

    // Unbind the storage buffer from ssbo
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
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