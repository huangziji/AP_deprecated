#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

static int loadShader1(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    { // detach shaders
        GLsizei numShaders;
        GLuint shaders[5];
        glGetAttachedShaders(prog, 5, &numShaders, shaders);
        for (int i=0; i<numShaders; i++) {
            glDetachShader(prog, shaders[i]);
        }
    }
    { // fragment shader
        const char *string[] = { source };
        const GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, string, NULL);
        glCompileShader(fs);
        int success;    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            int length; glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
            char message[length]; glGetShaderInfoLog(fs, length, &length, message);
            fprintf(stderr, "ERROR: fail to compile fragment shader. file %s\n%s\n", filename, message);
            return 0;
        }
        glAttachShader(prog, fs);
        glDeleteShader(fs);
    }
    { // vertex shader
        const char vsSource[] = R"(#version 300 es
        precision mediump float;
        void main() {
            vec2 UV = vec2(gl_VertexID&1, gl_VertexID/2);
            gl_Position = vec4(UV*2.-1., 0, 1);
        }
        )";

        const char *string[] = { vsSource };
        const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, string, NULL);
        glCompileShader(vs);
        int success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        assert(success);
        glAttachShader(prog, vs);
        glDeleteShader(vs);
    }

    glLinkProgram(prog);
    glValidateProgram(prog);
    return 1;
}

static int loadShader2(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    { // detach shaders
        GLsizei numShaders;
        GLuint shaders[5];
        glGetAttachedShaders(prog, 5, &numShaders, shaders);
        for (int i=0; i<numShaders; i++) {
            glDetachShader(prog, shaders[i]);
        }
    }
    { // fragment shader
        const char *string[] = { version, "#define _FS\n", source };
        const GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 3, string, NULL);
        glCompileShader(fs);
        int success;    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            int length; glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
            char message[length]; glGetShaderInfoLog(fs, length, &length, message);
            fprintf(stderr, "ERROR: fail to compile fragment shader. file %s\n%s\n", filename, message);
            return 0;
        }
        glAttachShader(prog, fs);
        glDeleteShader(fs);
    }
    { // vertex shader
        const char *string[] = { version, "#define _VS\n", source };
        const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 3, string, NULL);
        glCompileShader(vs);
        int success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            int length; glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &length);
            char message[length]; glGetShaderInfoLog(vs, length, &length, message);
            fprintf(stderr, "ERROR: fail to compile vertex shader. file %s\n%s\n", filename, message);
            return 0;
        }
        glAttachShader(prog, vs);
        glDeleteShader(vs);
    }

    glLinkProgram(prog);
    glValidateProgram(prog);
    return 1;
}

static void errorCallback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main(int argc, char *argv[])
{
const int RES_X = 16*40, RES_Y = 9*40;
    GLFWwindow *window1;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwSetErrorCallback(errorCallback);

    { // window #1
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);

        window1 = glfwCreateWindow(RES_X, RES_Y, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
        glfwSwapInterval(1);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    }

    if (0)
    { // texture octree
    const char source[] = R"(#version 310 es
layout (local_size_x=8, local_size_y=8, local_size_z=8) in;
layout (std430, binding = 0) buffer INPUT {
    int outData[];
};
void main() {
    ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
    outData[p.x + p.y + p.z] = p.x + p.y + p.z;
}
)";
    const GLuint prog3 = glCreateProgram();
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    const char *string[] = { source };
    glShaderSource(cs, 1, string, NULL);
    glCompileShader(cs);
    int success;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length; glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &length);
        char message[length]; glGetShaderInfoLog(cs, length, &length, message);
        fprintf(stderr, "ERROR: fail to compile compute shader. file \n%s\n", message);
    }
    glAttachShader(prog3, cs);
    glDeleteShader(cs);
    glLinkProgram(prog3);
    glValidateProgram(prog3);

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 1024, NULL, GL_STATIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    glUseProgram(prog3);
    glDispatchCompute(1,1,1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    char data[1024];
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    const GLuint prog1 = glCreateProgram();
    assert(loadShader1(prog1, "../base.frag"));
    const GLuint prog2 = glCreateProgram();
    assert(loadShader2(prog2, "../base.glsl"));

    GLint iChannel0 = glGetUniformLocation(prog1, "iChannel0");
    glProgramUniform1i(prog1, iChannel0, 0);
    GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
    glProgramUniform1i(prog1, iChannel1, 1);

    // framebuffer
    GLuint fbo, tex, tex2;
    {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &tex2); // clean up as program ends
        glBindTexture(GL_TEXTURE_2D, tex2);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, RES_X, RES_Y);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &fbo); // clean up as program ends
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        GLuint cbo;
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);

        GLuint ibo;
        uint8_t data[] = {0,1,2,3,4,5,6,7,8,9,10,11,12};
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ARRAY_BUFFER, ibo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 1, 0);
        glVertexAttribDivisor(1, 1);

        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 1024, NULL, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
    }

    typedef int (*plugFunction)(float);
    plugFunction mainGeometry, mainAnimation;
    const char *libraryFilename = "libmodule.so";
    void *libraryHandle = NULL;
    long lastModTime;
    struct stat libStat;
    int drawCount;

    while (!glfwWindowShouldClose(window1))
    {
        stat(libraryFilename, &libStat);
        if (libStat.st_mtime != lastModTime) {
            lastModTime = libStat.st_mtime;
            printf("INFO: reloading file %s\n", libraryFilename);

            usleep(100000);
            if (libraryHandle) {
                assert(!dlclose(libraryHandle));
            }
            libraryHandle = dlopen(libraryFilename, RTLD_NOW);
            assert(libraryHandle);
            mainAnimation = (plugFunction)dlsym(libraryHandle, "mainAnimation");
            assert(mainAnimation);
            mainGeometry  = (plugFunction)dlsym(libraryHandle, "mainGeometry");
            assert(mainGeometry);
            drawCount = mainGeometry(0);
        }

        float t = glfwGetTime();
        const float fov = 1.2;
        struct { float x,y,z; }ro, ta;
        ta = {0,0,0};
        ro = {5,2,5};
        //ro = {sin(t)*5,2,cos(t)*5};
        ro.x += ta.x, ro.y += ta.y, ro.z += ta.z;
        mainAnimation(t);

        const float data[16] = {
            RES_X, RES_Y, 0, 0,
            t, 0, fov, 0,
            ro.x, ro.y, ro.z, 0,
            ta.x, ta.y, ta.z, 0,
        };

        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), data);

        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0,0, RES_X, RES_Y);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog2);
        glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, NULL, drawCount, 0);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        //glReadBuffer(GL_COLOR_ATTACHMENT0);
        //glDrawBuffer(GL_BACK);
        //glBlitFramebuffer(0,0,RES_X,RES_Y,0,0,RES_X,RES_Y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window1);
        glfwPollEvents();
    }

    // Unload the library when done
    if (dlclose(libraryHandle)) {
        fprintf(stderr, "ERROR: fail to close file %s\n", libraryFilename);
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
    return 0;
}
