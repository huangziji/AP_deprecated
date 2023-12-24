#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/stat.h>

static int loadShader1(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
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
            return 2;
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
    return 0;
}

static int loadShader2(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
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
            return 2;
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
            return 2;
        }
        glAttachShader(prog, vs);
        glDeleteShader(vs);
    }

    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

static void errorCallback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main(int argc, char *argv[])
{
    const int RES_X = 16*40, RES_Y = 9*40;
    GLFWwindow *window1;

#define ENABLE_FFMPEG 0
#if ENABLE_FFMPEG
    const char cmd[] = "LD_LIBRARY_PATH=/usr/bin ffmpeg -r 60 -f rawvideo -pix_fmt rgb24 -s 640x360"
                       " -i pipe: -c:v libx264 -c:a aac"
                       " -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip 1.mp4";
    FILE *pipe = popen(cmd, "w");
#endif

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwSetErrorCallback(errorCallback);

    { // window #1
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);

        window1 = glfwCreateWindow(RES_X, RES_Y, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
        glfwSetWindowAttrib(window1, GLFW_FLOATING, GLFW_TRUE);
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

    // framebuffer
    GLuint fbo, tex1, tex2;
    {
        glGenTextures(1, &tex1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &tex2);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, RES_X, RES_Y);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex1, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    {
        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        GLuint vao, vbo, ibo, cbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
    }

    typedef int (plugFunction1)(void);
    typedef void (plugFunction2)(float*, float);
    plugFunction1 *mainGeometry;
    plugFunction2 *mainAnimation;
    const char *libraryFilename = "libmodule.so";
    void *libraryHandle = NULL;
    long lastModTime;
    struct stat libStat;
    int drawCount;

    long prog1_lastModTime;
    const char prog1_filename[] = "../base.frag";
    const GLuint prog1 = glCreateProgram();
    const GLuint prog2 = glCreateProgram();
    assert(loadShader2(prog2, "../base.glsl") == 0);

    while (!glfwWindowShouldClose(window1))
    {
        stat(prog1_filename, &libStat);
        if (prog1_lastModTime != libStat.st_mtime) {
            int err = loadShader1(prog1, prog1_filename);
            if (err == 1) {assert(0); continue;}
            prog1_lastModTime = libStat.st_mtime;

            GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
            glProgramUniform1i(prog1, iChannel1, 1);
        }

        stat(libraryFilename, &libStat);
        if (libStat.st_mtime != lastModTime) {
            if (libraryHandle) {
                assert(dlclose(libraryHandle) == 0);
            }
            libraryHandle = dlopen(libraryFilename, RTLD_LAZY);
            if (!libraryHandle) continue;

            printf("INFO: reloading file %s\n", libraryFilename);
            lastModTime = libStat.st_mtime;

            mainAnimation = (plugFunction2*)dlsym(libraryHandle, "mainAnimation");
            assert(mainAnimation);
            mainGeometry  = (plugFunction1*)dlsym(libraryHandle, "mainGeometry");
            assert(mainGeometry);
            drawCount = mainGeometry();
        }

        double xpos, ypos;
        glfwGetCursorPos(window1, &xpos, &ypos);
        const float t = glfwGetTime();
        const float fov = 1.2;
        float data[16] = { RES_X, RES_Y, float(xpos), float(ypos), t, 0, fov, 0, };
        mainAnimation(data + 8, t);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), data);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0,0, RES_X, RES_Y);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog2);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, drawCount, 0);

        if (data[7] > 0.5) {
            glLineWidth(1.0);
            glPointSize(2.0);
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, drawCount, 0);
            glMultiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_SHORT, NULL, drawCount, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

//        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
//        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//        glReadBuffer(GL_COLOR_ATTACHMENT0);
//        glDrawBuffer(GL_BACK);
//        glBlitFramebuffer(0,0,RES_X,RES_Y,0,0,RES_X,RES_Y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window1);
        glfwPollEvents();

#if ENABLE_FFMPEG
        static char buffer[RES_X*RES_Y*3];
        glReadPixels(0,0, RES_X, RES_Y, GL_RGB, GL_UNSIGNED_BYTE, buffer);
        fwrite(buffer, sizeof(buffer), 1, pipe);
#endif
    }

#if ENABLE_FFMPEG
    pclose(pipe);
#endif

    assert(dlclose(libraryHandle) == 0);
    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
    return 0;
}
