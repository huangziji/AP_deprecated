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

    for (int i=0; i<2; i++)
    {
        const GLenum shaderType[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
        const char *string[] = { version, i==0?"#define _VS\n":"#define _FS\n", source };
        const GLuint sha = glCreateShader(shaderType[i]);
        glShaderSource(sha, 3, string, NULL);
        glCompileShader(sha);
        int success;    glGetShaderiv(sha, GL_COMPILE_STATUS, &success);
        if (!success) {
            int length; glGetShaderiv(sha, GL_INFO_LOG_LENGTH, &length);
            char message[length]; glGetShaderInfoLog(sha, length, &length, message);
            fprintf(stderr, "ERROR: fail to compile %s shader. file %s\n%s\n",
                i==0?"vertex":"fragment", filename, message);
            return 2;
        }
        glAttachShader(prog, sha);
        glDeleteShader(sha);
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
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
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

    GLuint fbo;
    {
        GLuint tex1, tex2;
        glGenTextures(1, &tex1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, RES_X, RES_Y);
        glGenTextures(1, &tex2);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, RES_X, RES_Y);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex1, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex2);

        float data[16] = { RES_X,RES_Y,0,0, 1,1,1,1, };
        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof data, data, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        GLuint vao, vbo, ibo, cbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);
    }

    const GLuint prog2 = glCreateProgram();
    assert(loadShader2(prog2, "../base.glsl") == 0);
    GLint enableWireFrame = glGetUniformLocation(prog2, "enableWireFrame");

    const GLuint prog1 = glCreateProgram();
    const char *shaderFilename="../base.frag";
    const char *libraryFilename="libmodule.so";
    long lastModTime1, lastModTime2;
    void *libraryHandle = NULL;
    struct stat libStat;

    typedef int (plugFunction1)(void);
    typedef int (plugFunction2)(float);
    plugFunction1 *mainGeometry;
    plugFunction2 *mainAnimation;
    int drawCount = 0;
    int wireFrameMode = 0;

    while (!glfwWindowShouldClose(window1))
    {
        int err;
        err = stat(shaderFilename, &libStat);
        if (err == 0 && lastModTime1 != libStat.st_mtime) {
            err = loadShader1(prog1, shaderFilename);
            if (err != 1) {
                printf("INFO: reloading file %s\n", shaderFilename);
                lastModTime1 = libStat.st_mtime;
                GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
                glProgramUniform1i(prog1, iChannel1, 1);
                GLint iChannel2 = glGetUniformLocation(prog1, "iChannel2");
                glProgramUniform1i(prog1, iChannel2, 2);
            }
        }

        err = stat(libraryFilename, &libStat);
        if (err == 0 && lastModTime2 != libStat.st_mtime) {
            if (libraryHandle) {
                assert(dlclose(libraryHandle) == 0);
            }
            libraryHandle = dlopen(libraryFilename, RTLD_NOW);
            if (!libraryHandle) continue;
            printf("INFO: reloading file %s\n", libraryFilename);
            lastModTime2 = libStat.st_mtime;
            mainAnimation = (plugFunction2*)dlsym(libraryHandle, "mainAnimation");
            assert(mainAnimation);
            mainGeometry  = (plugFunction1*)dlsym(libraryHandle, "mainGeometry");
            assert(mainGeometry);
            drawCount = mainGeometry();
        }

        wireFrameMode = mainAnimation(glfwGetTime());

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
        glProgramUniform1f(prog2, enableWireFrame, 0);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, drawCount, 0);

        if (wireFrameMode) {
            glPointSize(2.0);
            glLineWidth(1.0);
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glProgramUniform1f(prog2, enableWireFrame, 1);
            glMultiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_SHORT, NULL, drawCount, 0);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, drawCount, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT);
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
