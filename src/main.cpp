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
    // File stream to load shader at runtime
    std::ifstream fs;
    fs.open(fName, std::ifstream::in);

    if(fs.fail()){
        printf("Failed to open file \'%s\'\n", fName);
    }

    // get the length of the file
    fs.seekg(0, fs.end);
    GLint len = fs.tellg();
    fs.seekg(0, fs.beg);

    // Read the whole file into memory
    char* src = new char[len];
    fs.read(src, len);

    // Get a shader handle from OpenGL
    unsigned int shader = glCreateShader(shaderType);
    // provide the GLSL source and compile it
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    // We can delete the memory we allocated for the shader source, gpu has it now
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

    // Load an OpenGL loader
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
    glClearColor(1.0, 0.0, 0.0, 0.0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);
    glDepthFunc(GL_GREATER);

    return true;
}

void loop(){
    GLuint fb_tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &fb_tex);
    glTextureParameteri(fb_tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(fb_tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(fb_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(fb_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(fb_tex, 1, GL_RGBA32F, 400, 400);

    GLuint fb_depthBuf;
    glCreateTextures(GL_TEXTURE_DEPTH, 1, &fb_depthBuf);
    glTextureParameteri(fb_depthBuf, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(fb_depthBuf, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(fb_depthBuf, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(fb_depthBuf, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(fb_depthBuf, 1, GL_DEPTH_COMPONENT32F, 400, 400);

    GLuint fbo;
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, fb_tex, 0);
    glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, fb_depthBuf, 0);
    glNamedFramebufferDrawBuffer(fbo, GL_COLOR_ATTACHMENT0);

    GLuint err = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
    if(err != GL_FRAMEBUFFER_COMPLETE){
        printf("Error in fbo: %x\n", err);
    }

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
        
        { { -.50, -.50, .5}, { 0,  0} },
        { {  .50,  .50, .5}, { 0,  0} },
        { { -.50,  .50, .5}, { 0,  0} },
    };

    // Simple anon struct to manage image files and what texture to bind them to
    const struct{
        const char* img;
        const GLenum tex;
    } images[] = {
        {"assets/container.jpg", 0},
        {"assets/bird.jpg", 1},
    };

    // Generate texture objects for each of  the images
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

    // Generate buffer for vert data
    GLuint ssbo;
    glCreateBuffers(1, &ssbo);
    glNamedBufferStorage(ssbo, sizeof(triangleVerts), triangleVerts, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // Compile and link shaders
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

    // Get location of `time` uniform in the shader program
    const auto timeLoc = glGetUniformLocation(shaderProg, "time");

    float bgColor[] = {1.0, 1.0, 0.0, 1.0};
    float depthClear = 0;
    while(!glfwWindowShouldClose(window)){
        // Clear the render buffer
        glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &depthClear);
        glClearNamedFramebufferfv(fbo, GL_COLOR, 0, bgColor);
        // Bind the frame buffer so we can draw to it
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        
        // Set the shader to use
        glUseProgram(shaderProg);
        glUniform1f(timeLoc, glfwGetTime());
        // Draw the triangle
        glDrawArrays(GL_TRIANGLES, 0, 9);

        glBlitNamedFramebuffer(fbo, 0, 0, 0, 400, 400, 0, 0, 400, 400, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBlitNamedFramebuffer(fbo, 0, 0, 0, 400, 400, 0, 0, 400, 400, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // Swap render and display buffers
        glfwSwapBuffers(window);

        glfwPollEvents();
    }
    

    // Unbind the storage buffer from ssbo
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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