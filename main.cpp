#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/stat.h>

static int _mainAnimation(float) { return 0; }

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
    glfwSetErrorCallback(errorCallback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    { // window1
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

        GLuint vao, ibo, cbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cbo);

        GLuint vbo1;
        glGenBuffers(1, &vbo1);
        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    }

    int loadShader1(GLuint, const char*);
    int loadShader2(GLuint, const char*);

    const GLuint prog3 = glCreateProgram();
    assert(0 == loadShader2(prog3, "../line.glsl"));

    const GLuint prog2 = glCreateProgram();
    assert(0 == loadShader2(prog2, "../base.glsl"));
    GLint enableWireFrame = glGetUniformLocation(prog2, "enableWireFrame");

    const GLuint prog1 = glCreateProgram();
    const char *shaderFilename="../base.frag";
    long lastModTime1;

    typedef int (plugFunction1)(float);
    typedef int (plugFunction2)(void);
    typedef int (plugFunction3)(void);
    plugFunction1 *mainAnimation = _mainAnimation;
    plugFunction2 *mainGeometry;
    plugFunction3 *mainGizmo;
    int result1 = 0, result2 = 0, result3 = 0;

    void *libraryHandle = NULL;
    const char *libraryFilename="libModule.so";
    long lastModTime2;

    while (!glfwWindowShouldClose(window1))
    {
        {
            struct stat libStat;
            int err = stat(shaderFilename, &libStat);
            if (err == 0 && lastModTime1 != libStat.st_mtime)
            {
                err = loadShader1(prog1, shaderFilename);
                if (err != 1)
                {
                    printf("INFO: reloading file %s\n", shaderFilename);
                    lastModTime1 = libStat.st_mtime;
                    GLint iChannel1 = glGetUniformLocation(prog1, "iChannel1");
                    glProgramUniform1i(prog1, iChannel1, 1);
                    GLint iChannel2 = glGetUniformLocation(prog1, "iChannel2");
                    glProgramUniform1i(prog1, iChannel2, 2);
                }
            }
        }

        {
            struct stat libStat;
            int err = stat(libraryFilename, &libStat);
            if (err == 0 && lastModTime2 != libStat.st_mtime)
            {
                if (libraryHandle)
                {
                    assert(dlclose(libraryHandle) == 0);
                }
                libraryHandle = dlopen(libraryFilename, RTLD_NOW);
                if (!libraryHandle) continue;
                printf("INFO: reloading file %s\n", libraryFilename);
                lastModTime2 = libStat.st_mtime;
                mainAnimation = (plugFunction1*)dlsym(libraryHandle, "mainAnimation");
                assert(mainAnimation);

                mainGeometry = (plugFunction2*)dlsym(libraryHandle, "mainGeometry");
                if (mainGeometry) result2 = mainGeometry();
                mainGizmo = (plugFunction3*)dlsym(libraryHandle, "mainGizmo");
                if (mainGizmo) result3 = mainGizmo();
            }
            float t = glfwGetTime();
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float)*2, sizeof(float), &t);
            result1 = mainAnimation(t);
        }

        glClearColor(0,0,0,1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0,0, RES_X, RES_Y);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog2);
        glProgramUniform1f(prog2, enableWireFrame, 0);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, result2, 0);

        if (result1)
        {
            glPointSize(2.0);
            glLineWidth(1.0);
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glProgramUniform1f(prog2, enableWireFrame, 1);
            glMultiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_SHORT, NULL, result2, 0);
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, NULL, result2, 0);
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

        glUseProgram(prog3);
        glDrawArrays(GL_LINES, 0, result3);

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

    //assert(dlclose(libraryHandle) == 0);

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
    glfwDestroyWindow(window1);
    glfwTerminate();
    return 0;
}
